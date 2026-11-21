'use strict';

const host = document.getElementById('editor');
const post = message => window.chrome?.webview?.postMessage(message);
const dialog = document.querySelector('dialog');
let editor;
let current = '';
let revision = 0;
let previewTimer;
let pendingUpload;
let dark = false;

function update(value, notify = true) {
    current = value;
    if (notify) post({type: 'input', markdown: value, revision});
    clearTimeout(previewTimer);
    previewTimer = setTimeout(() => window.vsrRenderDocument({
        blocks: [{markdown: current}], base: 'https://www1.arxchem.top/', key: 'draft', dark
    }), 120);
}

function theme(value) {
    dark = value;
    editor.setTheme(dark ? 'dark' : 'classic', dark ? 'dark' : 'light', dark ? 'native' : 'github');
    window.vsrDocumentTheme(dark);
}

const tools = document.getElementById('rich-tools');
tools.addEventListener('mousedown', event => {
    if (event.target.tagName === 'BUTTON') event.preventDefault();
});
document.getElementById('wide').onclick = () => {
    const selected = editor.getSelection();
    const marker = '{table:scroll}';
    editor.insertValue(selected.includes(marker) ? selected.replace(marker + '\n\n', '').replace(marker, '') :
        marker + '\n\n' + (selected.trim() ? selected : '| 列1 | 列2 | 列3 |\n| --- | --- | --- |\n| 内容 | 内容 | 内容 |'));
};
for (const kind of ['color', 'blackout']) {
    document.getElementById(kind).onclick = () => {
        const selected = editor.getSelection();
        const value = dialog.querySelector('input');
        const body = dialog.querySelector('textarea');
        value.type = kind === 'color' ? 'color' : 'text';
        value.value = kind === 'color' ? '#d14b4b' : '你知道的太多了';
        body.value = selected || (kind === 'color' ? '彩色文字' : '');
        dialog.querySelector('span').textContent = kind === 'color' ? '文字颜色' : '黑幕提示';
        dialog.returnValue = '';
        dialog.showModal();
        dialog.onclose = () => {
            if (dialog.returnValue !== 'ok' || !body.value.trim()) return;
            const marker = value.value.replace(/[{}\r\n]/g, '').trim() || '隐藏文字';
            const lines = body.value.split('\n');
            editor.insertValue(lines.map(line => {
                if (!line.trim()) return line;
                return line.replace(/^(\s*(?:(?:#{1,6}|>|[-+*]|[0-9]+\.)\s+)?)(.*)$/,
                    (_, prefix, text) => prefix + '{' + kind + ':' + marker + '}' + text + '{/' + kind + '}');
            }).join('\n'));
        };
    };
}
document.getElementById('import').addEventListener('change', async event => {
    const file = event.target.files[0];
    if (file?.size <= 4 * 1024 * 1024) editor.insertValue(await file.text());
    event.target.value = '';
});
document.getElementById('formula').onclick = () => editor.insertValue('$' + (editor.getSelection() || 'x^2') + '$');

async function upload(files) {
    for (const file of files) {
        if (!/\.(png|jpe?g|gif|webp|avif)$/i.test(file.name) || file.size > 10 * 1024 * 1024)
            return '请选择 10 MiB 以内的 PNG、JPEG、GIF、WebP 或 AVIF。';
        const data = await new Promise(resolve => {
            const reader = new FileReader();
            reader.onload = () => resolve(reader.result.split(',')[1]);
            reader.onerror = () => resolve('');
            reader.readAsDataURL(file);
        });
        if (!data) return '无法读取图片。';
        const result = await new Promise(resolve => {
            pendingUpload = resolve;
            post({type: 'upload', name: file.name, data});
        });
        if (result.error) return result.error;
        editor.insertValue('![图片](<' + result.url + '>)');
    }
    return null;
}

window.vsrEditorCommand = data => {
    if (data.type === 'load') {
        revision = data.revision;
        if (current !== data.markdown) editor.setValue(data.markdown, true);
        update(data.markdown, false);
    } else if (data.type === 'theme') theme(data.dark);
    else if (data.type === 'lock') data.locked ? editor.disabled() : editor.enable();
    else if (data.type === 'uploaded') {
        pendingUpload?.(data);
        pendingUpload = undefined;
    }
};

try {
    editor = new Vditor(host.id, {
        cdn: '/vendor/vditor', lang: 'zh_CN', mode: 'sv', height: '100%', minHeight: 0,
        placeholder: '输入内容；支持实时预览、表格、任务列表、公式、代码块和图片…',
        cache: {enable: false},
        counter: {enable: true, type: 'markdown'},
        outline: {enable: false},
        preview: {mode: 'editor', delay: 120, math: {engine: 'KaTeX'},
            theme: {current: 'light', path: '/vendor/vditor/dist/css/content-theme'},
            markdown: {autoSpace: true, toc: true, footnotes: true, mark: true,
                gfmAutoLink: true, imageCaption: true, sanitize: true}},
        toolbar: ['emoji', 'headings', 'bold', 'italic', 'strike', 'link', '|', 'list', 'ordered-list',
            'check', '|', 'quote', 'line', 'code', 'inline-code', '|', 'upload', 'record', 'table', '|',
            'undo', 'redo', '|', 'fullscreen', 'edit-mode', {name: 'more', toolbar: ['outdent', 'indent', 'insert-before', 'insert-after']}],
        upload: {accept: 'image/png,image/jpeg,image/gif,image/webp,image/avif', handler: upload},
        input: value => update(value),
        blur: value => update(value),
        after() {
            host.querySelector('.vditor-toolbar').append(tools);
            tools.hidden = false;
            post({type: 'ready'});
        }
    });
} catch { post({type: 'error'}); }

let split = false;
const divider = document.getElementById('divider');
divider.addEventListener('pointerdown', event => {
    split = true;
    divider.setPointerCapture(event.pointerId);
});
divider.addEventListener('pointermove', event => {
    if (!split) return;
    const bounds = document.querySelector('.editor-columns').getBoundingClientRect();
    const width = Math.max(22, Math.min(78, (event.clientX - bounds.left) / bounds.width * 100));
    document.documentElement.style.setProperty('--editor-width', width + '%');
});
divider.addEventListener('lostpointercapture', () => { split = false; });


