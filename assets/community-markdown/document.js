'use strict';

const content = document.getElementById('document');
const lute = Lute.New();
lute.SetFootnotes(true);
lute.SetToC(true);
lute.SetMark(true);
lute.SetInlineMath(true);
lute.SetInlineMathAllowDigitAfterOpenMarker(true);
lute.SetSanitize(true);

// Bundled renderers may enhance a fenced block. Unknown or invalid blocks stay
// as source. The same rendering path is used by the reader and the Qt editor preview.
const fences = new Map();
window.vsrRegisterFence = (language, render) => fences.set(language.toLowerCase(), render);

const colorPattern = /^#(?:[0-9a-f]{3}|[0-9a-f]{6}|[0-9a-f]{8})$/i;

function encodeRichMarkers(line) {
    const pending = [];
    const paired = [];
    const markers = /(`+).*?\1|(?<!\\)\{(?:(color|blackout):([^{}\r\n]{1,120})|\/(color|blackout))\}/gi;
    for (const token of line.matchAll(markers)) {
        if (token[1]) continue;
        if (token[2]) {
            const kind = token[2].toLowerCase();
            const value = token[3].trim();
            if ((kind === 'color' && !colorPattern.test(value)) || !value) continue;
            const escaped = value.replaceAll('&', '&amp;').replaceAll('"', '&quot;')
                .replaceAll('<', '&lt;').replaceAll('>', '&gt;');
            pending.push({ kind, start: token.index, end: token.index + token[0].length,
                html: '<span data-vsr-' + kind + '="' + escaped + '">' });
        } else if (pending.at(-1)?.kind === token[4].toLowerCase()) {
            const open = pending.pop();
            paired.push(open, { start: token.index, end: token.index + token[0].length, html: '</span>' });
        }
    }
    for (const marker of paired.sort((a, b) => b.start - a.start))
        line = line.slice(0, marker.start) + marker.html + line.slice(marker.end);
    return line;
}

function normalize(source) {
    const text = source
        .replace(/!\[\[([^\]\r\n]*)\]\]\\?\(\[(https?:\/\/[^\]\s]+)\]\((https?:\/\/[^)\s]+)\)\)/gi, '![$1]($3)')
        .replace(/!\[\[([^\]\r\n]*)\]\]\\?\((https?:\/\/[^)\s]+)\)/gi, '![$1]($2)')
        .replace(/http:\/\/(i[012]\.hdslb\.com)\//g, 'https://$1/')
        .replace(/&#x20;|&#32;/gi, ' ');
    let fence = '';
    return text.split('\n').map(line => {
        const mark = line.match(/^\s{0,3}(`{3,}|~{3,})(.*)$/);
        if (fence) {
            if (mark && mark[1][0] === fence[0] && mark[1].length >= fence.length && !mark[2].trim()) fence = '';
            return line;
        }
        if (mark) {
            fence = mark[1];
            return line;
        }
        if (/^\s*\\[\[\]]\s*$/.test(line)) return '$$';
        const math = line.replace(/(`+).*?\1|\\\((.*?)\\\)|\\\[(.*?)\\\]/g,
            (whole, code, inline, display) => code ? whole : inline !== undefined ? '$' + inline + '$' : '\n$$\n' + display + '\n$$\n');
        return encodeRichMarkers(math);
    }).join('\n');
}

function renderMath(block) {
    for (const formula of block.querySelectorAll('.language-math')) {
        const display = formula.tagName === 'DIV' || formula.parentElement.tagName === 'PRE';
        const target = document.createElement(display ? 'div' : 'span');
        if (display) target.className = 'math-display';
        try {
            katex.render(formula.textContent, target, { displayMode: display, trust: false, maxExpand: 1000, maxSize: 20 });
        } catch {
            target.classList.add('formula-error');
            target.textContent = formula.textContent;
        }
        (formula.parentElement.tagName === 'PRE' ? formula.parentElement : formula).replaceWith(target);
    }
}

function renderInlineStyles(block) {
    for (const span of block.querySelectorAll('span[data-vsr-color]')) {
        const value = span.getAttribute('data-vsr-color');
        if (colorPattern.test(value)) span.style.color = value;
    }
    for (const span of block.querySelectorAll('span[data-vsr-blackout]')) {
        span.classList.add('blackout');
        span.tabIndex = 0;
        span.title = span.getAttribute('data-vsr-blackout').slice(0, 120);
        span.setAttribute('role', 'note');
        span.addEventListener('click', event => {
            if (!span.classList.contains('revealed')) event.preventDefault();
            span.classList.toggle('revealed');
        });
        span.addEventListener('keydown', event => {
            if (event.target === span && (event.key === 'Enter' || event.key === ' ')) {
                event.preventDefault();
                span.classList.toggle('revealed');
            }
        });
    }
}

function renderMarkdown(block, source, base, index) {
    const fragment = DOMPurify.sanitize(lute.Md2HTML(normalize(source)), {
        RETURN_DOM_FRAGMENT: true,
        USE_PROFILES: { html: true },
        FORBID_TAGS: ['style', 'form', 'button', 'object', 'embed', 'meta', 'base'],
        FORBID_ATTR: ['style', 'srcset', 'srcdoc', 'target', 'autoplay', 'name'],
        ADD_TAGS: ['iframe'],
        ADD_ATTR: ['data-target-id', 'data-vsr-color', 'data-vsr-blackout', 'data-src'],
        ALLOW_DATA_ATTR: false
    });
    const ids = new Map();
    for (const el of fragment.querySelectorAll('[id]')) {
        const original = el.id;
        el.id = 'vsr-block-' + index + '-' + original;
        ids.set(original, el.id);
    }
    for (const el of fragment.querySelectorAll('[href],[src]')) {
        const attribute = el.hasAttribute('href') ? 'href' : 'src';
        const raw = el.getAttribute(attribute);
        if (attribute === 'href' && raw.startsWith('#')) {
            if (ids.has(raw.slice(1))) el.setAttribute(attribute, '#' + ids.get(raw.slice(1)));
            continue;
        }
        try {
            const url = new URL(raw, base);
            const raster = el.tagName === 'IMG' && /^data:image\/(?:png|jpeg|gif|webp|avif);base64,/i.test(raw);
            if (!url.username && !url.password && (url.protocol === 'https:' ||
                (attribute === 'href' && url.protocol === 'http:') || raster)) {
                el.setAttribute(attribute, url.href);
                continue;
            }
        } catch {}
        el.removeAttribute(attribute);
    }
    for (const item of fragment.querySelectorAll('.vditor-toc span[data-target-id]')) {
        const id = ids.get(item.dataset.targetId);
        if (!id) continue;
        const link = document.createElement('a');
        link.href = '#' + id;
        link.textContent = item.textContent;
        item.replaceWith(link);
    }
    replaceCommunityFrames(fragment, base);
    const walker = document.createTreeWalker(fragment, NodeFilter.SHOW_TEXT);
    const lines = [];
    while (walker.nextNode()) {
        const node = walker.currentNode;
        const parent = node.parentElement;
        if (parent?.closest('p,td,th,li,blockquote,dd') && !parent.closest('pre,code,.language-math,.vditor-math'))
            lines.push(node);
    }
    for (const node of lines) {
        let value = node.nodeValue.replace(/\\(?:r\\n|[nrt])/g, (escape, offset) => {
            const command = node.nodeValue.slice(offset).match(/^\\([A-Za-z]+)/)?.[1];
            if (/^(nu|nabla|ne|neq|not|notin|newcommand|newline|tau|theta|times|text|textbf|textit|textrm|textsf|texttt|tfrac|tan|tanh|tag|top|to|rightarrow|right|rho|ref|renewcommand)$/.test(command || '')) return escape;
            return escape === '\\t' ? '\u00a0'.repeat(4) : '\n';
        });
        if (node.previousSibling?.nodeName === 'BR') value = value.replace(/^\r?\n/, '');
        if (node.nextSibling?.nodeName === 'BR') value = value.replace(/\r?\n$/, '');
        if (!/[\r\n\t]/.test(value) && value === node.nodeValue) continue;
        const replacement = document.createDocumentFragment();
        value.replaceAll('\t', '\u00a0'.repeat(4)).split(/\r\n?|\n/).forEach((line, index) => {
            if (index) replacement.append(document.createElement('br'));
            replacement.append(document.createTextNode(line));
        });
        node.replaceWith(replacement);
    }
    block.replaceChildren(fragment);
    renderMath(block);
    renderInlineStyles(block);
    for (const node of block.querySelectorAll('pre > code[class]')) {
        const language = [...node.classList].find(name => name.startsWith('language-'))?.slice(9).toLowerCase();
        const render = fences.get(language);
        if (!render) continue;
        try {
            const output = render(node.textContent, { base });
            if (output instanceof HTMLElement) node.parentElement.replaceWith(output);
        } catch {}
    }
    enhanceCommunityMarkdown(block, base);
}

window.vsrDocumentTheme = dark => {
    document.documentElement.dataset.theme = dark ? 'dark' : 'light';
};

let fontSize = 18;
try { fontSize = Math.max(14, Math.min(38, Number(localStorage.getItem('fontSize')) || 18)); } catch {}
function applySize() {
    document.documentElement.style.fontSize = fontSize + 'px';
}
applySize();
window.vsrDocumentZoom = step => {
    fontSize = step ? Math.max(14, Math.min(38, fontSize + step)) : 18;
    applySize();
    try { localStorage.setItem('fontSize', String(fontSize)); } catch {}
};
window.addEventListener('storage', event => {
    if (event.key !== 'fontSize') return;
    fontSize = Math.max(14, Math.min(38, Number(event.newValue) || 18));
    applySize();
});
window.addEventListener('wheel', event => {
    if (!event.ctrlKey || !event.deltaY) return;
    event.preventDefault();
    window.vsrDocumentZoom(event.deltaY < 0 ? 1 : -1);
}, { passive: false });

let documentKey = '';
window.vsrRenderDocument = ({ blocks, base, key, dark }) => {
    window.vsrDocumentTheme(dark);
    const position = documentKey === key ? window.scrollY : 0;
    documentKey = key;
    const articles = blocks.map(({ title, meta, markdown }, index) => {
        const article = document.createElement('article');
        if (title) {
            const heading = document.createElement(index ? 'h2' : 'h1');
            heading.className = 'thread-title';
            heading.textContent = title;
            article.append(heading);
        }
        if (meta) {
            const note = document.createElement('p');
            note.className = 'thread-meta';
            note.textContent = meta;
            article.append(note);
        }
        const block = document.createElement('div');
        renderMarkdown(block, markdown, base, index);
        article.append(block);
        return article;
    });
    disposeCommunityMarkdown();
    content.replaceChildren(...articles);
    window.scrollTo(0, position);
    return true;
};

const headings = () => [...content.querySelectorAll('h1,h2,h3,h4,h5,h6')].filter(h => !h.classList.contains('thread-title'));
window.vsrDocumentOutline = () => headings().map(h => ({
    level: Number(h.tagName[1]), title: h.textContent.trim()
}));
window.vsrScrollToHeading = index => headings()[index]?.scrollIntoView({block: 'start'});
