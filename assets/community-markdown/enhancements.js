'use strict';

const communityPdfLimit = 25 * 1024 * 1024;
const activeMedia = new Set();
let pdfLibrary;

function disposeCommunityMarkdown() {
    for (const dispose of activeMedia) dispose();
    activeMedia.clear();
}

function mediaUrl(raw, base) {
    if (!raw) return null;
    try {
        const url = new URL(raw, base);
        return url.protocol === 'https:' && !url.username && !url.password && !url.port ? url : null;
    } catch {
        return null;
    }
}

function firstPartyMedia(url) {
    return url.hostname === 'www1.arxchem.top' || /(^|\.)modelscope\.(cn|ai)$/.test(url.hostname)
        || url.hostname.endsWith('.aliyuncs.com');
}

function dragWideTable(wrapper) {
    let drag;
    wrapper.addEventListener('pointerdown', event => {
        if (event.button !== 0 || event.target.closest('a,input,button') || wrapper.scrollWidth <= wrapper.clientWidth) return;
        drag = { x: event.clientX, scroll: wrapper.scrollLeft, id: event.pointerId };
    });
    wrapper.addEventListener('pointermove', event => {
        if (!drag || drag.id !== event.pointerId || !(event.buttons & 1)) { drag = undefined; return; }
        const offset = event.clientX - drag.x;
        if (!wrapper.hasPointerCapture(drag.id) && Math.abs(offset) < 5) return;
        wrapper.setPointerCapture(drag.id);
        wrapper.classList.add('dragging-table');
        getSelection()?.removeAllRanges();
        wrapper.scrollLeft = drag.scroll - offset;
    });
    for (const type of ['pointerup', 'pointercancel', 'lostpointercapture'])
        wrapper.addEventListener(type, () => { drag = undefined; wrapper.classList.remove('dragging-table'); });
}

async function openPdfPreview(details, url, controls, canvas, status) {
    const controller = new AbortController();
    let task;
    let render;
    const dispose = () => {
        controller.abort();
        render?.cancel();
        if (task) void task.destroy().catch(() => {});
    };
    activeMedia.add(dispose);
    try {
        const response = await fetch(url, { credentials: 'omit', referrerPolicy: 'no-referrer', signal: controller.signal });
        const declared = Number(response.headers.get('Content-Length') || 0);
        if (!response.ok || !response.body || declared > communityPdfLimit) throw new Error('PDF could not be loaded');
        const stream = response.body.getReader();
        const chunks = [];
        let received = 0;
        for (;;) {
            const { done, value } = await stream.read();
            if (done) break;
            received += value.length;
            if (received > communityPdfLimit) {
                await stream.cancel();
                throw new Error('PDF is too large');
            }
            chunks.push(value);
        }
        const data = new Uint8Array(received);
        let offset = 0;
        for (const chunk of chunks) { data.set(chunk, offset); offset += chunk.length; }
        if (controller.signal.aborted) return;
        const root = new URL('./vendor/pdfjs/', location.href);
        pdfLibrary ??= import(new URL('pdf.min.mjs', root).href);
        const pdfjs = await pdfLibrary;
        if (controller.signal.aborted) return;
        pdfjs.GlobalWorkerOptions.workerSrc = new URL('pdf.worker.min.mjs', root).href;
        pdfjs.GlobalWorkerOptions.rendererSrc = new URL('pdf.renderer.min.mjs', root).href;
        task = pdfjs.getDocument({ data, useWasm: false,
            cMapUrl: new URL('cmaps/', root).href, cMapPacked: true,
            standardFontDataUrl: new URL('standard_fonts/', root).href });
        const pdf = await task.promise;
        let current = 1;
        const [previous, next] = controls.querySelectorAll('button');
        async function showPage(number) {
            previous.disabled = next.disabled = true;
            try {
                const page = await pdf.getPage(number);
                if (!details.isConnected || controller.signal.aborted) return;
                const natural = page.getViewport({ scale: 1 });
                const scale = Math.min(1.7, 1150 / natural.width, 1650 / natural.height);
                const viewport = page.getViewport({ scale });
                canvas.width = Math.ceil(viewport.width);
                canvas.height = Math.ceil(viewport.height);
                render = page.render({ canvas, canvasContext: canvas.getContext('2d'), viewport });
                await render.promise;
                current = number;
                canvas.hidden = false;
                status.textContent = '第 ' + current + ' / ' + pdf.numPages + ' 页';
                previous.disabled = current <= 1;
                next.disabled = current >= pdf.numPages;
            } catch {
                if (details.isConnected && !controller.signal.aborted) status.textContent = '当前页无法预览，可打开原 PDF。';
            }
        }
        previous.onclick = () => { void showPage(current - 1); };
        next.onclick = () => { void showPage(current + 1); };
        controls.hidden = false;
        await showPage(1);
    } catch {
        if (details.isConnected && !controller.signal.aborted) status.textContent = 'PDF 无法内嵌预览，可打开原链接。';
    }
}

function addPdfPreview(anchor, url) {
    const details = document.createElement('details');
    details.className = 'markdown-media pdf-preview';
    const summary = document.createElement('summary');
    summary.textContent = '预览 PDF · ' + (anchor.textContent.trim() || 'PDF');
    const status = document.createElement('p');
    status.className = 'markdown-media-status';
    status.textContent = '正在读取 PDF…';
    const controls = document.createElement('div');
    controls.className = 'pdf-controls';
    controls.hidden = true;
    for (const text of ['上一页', '下一页']) {
        const button = document.createElement('button');
        button.type = 'button';
        button.textContent = text;
        controls.append(button);
    }
    const canvas = document.createElement('canvas');
    canvas.hidden = true;
    details.append(summary, status, controls, canvas);
    details.addEventListener('toggle', () => {
        if (!details.open || details.dataset.started) return;
        details.dataset.started = '1';
        void openPdfPreview(details, url.href, controls, canvas, status);
    });
    const parent = anchor.closest('p');
    (parent || anchor).insertAdjacentElement('afterend', details);
}

function framedVideo(url) {
    return (url.hostname === 'player.bilibili.com' && url.pathname === '/player.html') ||
        (url.hostname === 'www.youtube-nocookie.com' && /^\/embed\/[A-Za-z0-9_-]+\/?$/.test(url.pathname));
}

function replaceCommunityFrames(fragment, base) {
    for (const frame of fragment.querySelectorAll('iframe')) {
        const url = mediaUrl(frame.getAttribute('src'), base);
        if (!url) { frame.remove(); continue; }
        const link = document.createElement('a');
        link.href = url.href;
        link.textContent = frame.getAttribute('title') || '打开嵌入内容';
        frame.replaceWith(link);
    }
}

function addFramedVideo(link, url) {
    const details = document.createElement('details');
    details.className = 'markdown-media';
    const summary = document.createElement('summary');
    summary.textContent = '播放嵌入视频';
    details.append(summary);
    details.addEventListener('toggle', () => {
        if (!details.open || details.querySelector('iframe')) return;
        const frame = document.createElement('iframe');
        frame.title = '嵌入视频';
        frame.className = 'markdown-video-frame';
        frame.referrerPolicy = 'no-referrer';
        frame.setAttribute('sandbox', 'allow-scripts allow-same-origin allow-presentation');
        frame.setAttribute('allow', 'fullscreen; picture-in-picture');
        frame.src = url.href;
        details.append(frame);
    });
    (link.closest('p') || link).insertAdjacentElement('afterend', details);
}

function enableImageViewer(image) {
    if (image.closest('a') || !image.getAttribute('src')) return;
    image.classList.add('zoomable-image');
    image.tabIndex = 0;
    image.setAttribute('role', 'button');
    image.setAttribute('aria-label', '放大图片' + (image.alt ? ' · ' + image.alt : ''));
    const show = () => {
        if (!image.naturalWidth) return;
        const dialog = document.createElement('dialog');
        dialog.className = 'markdown-image-viewer';
        const enlarged = document.createElement('img');
        enlarged.alt = image.alt;
        enlarged.src = image.currentSrc || image.src;
        const close = document.createElement('button');
        close.type = 'button';
        close.textContent = '关闭';
        close.onclick = () => dialog.close();
        const dispose = () => dialog.remove();
        activeMedia.add(dispose);
        dialog.addEventListener('close', () => { activeMedia.delete(dispose); dispose(); });
        dialog.append(close, enlarged);
        document.body.append(dialog);
        dialog.showModal();
    };
    image.addEventListener('click', event => { if (!event.defaultPrevented) show(); });
    image.addEventListener('keydown', event => {
        if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); show(); }
    });
}

function enhanceCommunityMarkdown(block, base) {
    for (const table of block.querySelectorAll('table')) {
        const marker = table.previousElementSibling;
        const wide = marker?.textContent.trim() === '{table:scroll}';
        if (wide) marker.remove();
        const wrapper = document.createElement('div');
        wrapper.className = wide ? 'table-wrap wide-table' : 'table-wrap';
        table.replaceWith(wrapper);
        wrapper.append(table);
        if (wide) dragWideTable(wrapper);
    }
    for (const paragraph of block.querySelectorAll('p')) {
        const mark = paragraph.textContent.trim().match(/^\{embed-page:(\/embedded\/[a-z0-9-]+\.html)\}$/i);
        if (!mark) continue;
        const link = document.createElement('a');
        link.className = 'markdown-media';
        link.href = new URL(mark[1], 'https://www1.arxchem.top/').href;
        link.textContent = '打开嵌入页面 · ' + mark[1].split('/').pop();
        paragraph.replaceWith(link);
    }
    for (const code of block.querySelectorAll('pre > code')) {
        const raw = code.textContent;
        const language = [...code.classList].find(name => name.startsWith('language-'))?.slice(9);
        if (language && window.hljs?.getLanguage(language) && raw.length <= 16000) {
            code.textContent = raw;
            window.hljs.highlightElement(code);
        }
        const count = raw.trimEnd().split('\n').length;
        if (count < 2 || count > 2000) continue;
        const gutter = document.createElement('span');
        gutter.className = 'code-gutter';
        gutter.setAttribute('aria-hidden', 'true');
        gutter.textContent = Array.from({length: count}, (_, index) => index + 1).join('\n');
        code.parentElement.classList.add('numbered-code');
        code.before(gutter);
    }
    for (const node of block.querySelectorAll('video,audio')) {
        node.controls = true;
        node.preload = 'none';
        if (node.tagName === 'VIDEO') node.playsInline = true;
    }
    for (const image of block.querySelectorAll('img')) {
        const lazy = mediaUrl(image.getAttribute('data-src'), base);
        if (!image.getAttribute('src') && lazy) image.src = lazy.href;
        image.removeAttribute('data-src');
        image.loading = 'lazy';
        image.decoding = 'async';
        enableImageViewer(image);
    }
    for (const link of block.querySelectorAll('a[href]')) {
        const url = mediaUrl(link.getAttribute('href'), base);
        if (!url) continue;
        if (framedVideo(url)) { addFramedVideo(link, url); continue; }
        if (!firstPartyMedia(url)) continue;
        if (/\.pdf$/i.test(url.pathname)) { addPdfPreview(link, url); continue; }
        const paragraph = link.parentElement;
        if (paragraph.tagName !== 'P' || paragraph.textContent.trim() !== link.textContent.trim()) continue;
        const video = /\.(?:mp4|webm|mov)$/i.test(url.pathname);
        if (!video && !/\.(?:mp3|m4a|ogg|wav)$/i.test(url.pathname)) continue;
        const player = document.createElement(video ? 'video' : 'audio');
        player.controls = true;
        player.preload = 'none';
        player.src = url.href;
        paragraph.after(player);
    }
}
