#include "runtime/PortableLayout.h"
#include "ui/CommunityMarkdownEditor.h"
#include "ui/CommunityWebView.h"

#include <QBuffer>
#include <QColorDialog>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShowEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace vsr {
namespace {

constexpr auto imageLimit = 10 * 1024 * 1024;

class MarkdownSource final : public QPlainTextEdit {
public:
    explicit MarkdownSource(QWidget *parent) : QPlainTextEdit(parent)
    {
        auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(12);
        setFont(font);
    }

    std::function<void(const QString &, const QByteArray &)> pasteImage;

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (!(event->modifiers() & Qt::ControlModifier) || !event->angleDelta().y()) {
            QPlainTextEdit::wheelEvent(event);
            return;
        }
        const auto points = font().pointSize();
        if (event->angleDelta().y() > 0 && points < 28) zoomIn();
        else if (event->angleDelta().y() < 0 && points > 9) zoomOut();
        event->accept();
    }

    void insertFromMimeData(const QMimeData *data) override
    {
        if (!isReadOnly() && data->hasImage() && pasteImage) {
            const auto image = qvariant_cast<QImage>(data->imageData());
            if (!image.isNull()) {
                QBuffer buffer;
                buffer.open(QIODevice::WriteOnly);
                if (image.save(&buffer, "PNG")) pasteImage(QStringLiteral("clipboard.png"), buffer.data());
                return;
            }
        }
        QPlainTextEdit::insertFromMimeData(data);
    }
};

}

CommunityMarkdownEditor::CommunityMarkdownEditor(Upload upload, QWidget *parent)
    : QWidget(parent), upload_(std::move(upload))
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    views_ = new QStackedWidget(this);
    outer->addWidget(views_, 1);
    basic_ = new QWidget(views_);
    views_->addWidget(basic_);
    auto *layout = new QVBoxLayout(basic_);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *row = new QHBoxLayout;
    layout->addLayout(row);
    const auto action = [&](const QString &name, const QString &tooltip, auto invoke) {
        auto *button = new QPushButton(name, this);
        button->setToolTip(tooltip);
        row->addWidget(button);
        connect(button, &QPushButton::clicked, this, std::move(invoke));
        return button;
    };
    auto *format = new QPushButton(QStringLiteral("格式"), this);
    row->addWidget(format);
    auto *formatMenu = new QMenu(format);
    format->setMenu(formatMenu);
    auto *insert = new QPushButton(QStringLiteral("插入"), this);
    row->addWidget(insert);
    auto *insertMenu = new QMenu(insert);
    insert->setMenu(insertMenu);
    const auto snippet = [this](QMenu *menu, const QString &name, const QString &before,
                                const QString &placeholder, const QString &after = {}) {
        menu->addAction(name, this, [this, before, placeholder, after] {
            const auto selected = selection();
            insertMarkup(before, selected.isEmpty() ? placeholder : selected, after);
        });
    };
    snippet(formatMenu, QStringLiteral("标题"), "\n## ", QStringLiteral("标题"), "\n");
    snippet(formatMenu, QStringLiteral("粗体"), "**", QStringLiteral("文字"), "**");
    snippet(formatMenu, QStringLiteral("斜体"), "*", QStringLiteral("文字"), "*");
    snippet(formatMenu, QStringLiteral("删除线"), "~~", QStringLiteral("文字"), "~~");
    snippet(formatMenu, QStringLiteral("高亮"), "==", QStringLiteral("文字"), "==");
    snippet(formatMenu, QStringLiteral("引用"), "\n> ", QStringLiteral("引用"), "\n");
    snippet(formatMenu, QStringLiteral("无序列表"), "\n- ", QStringLiteral("列表项"), "\n");
    snippet(formatMenu, QStringLiteral("有序列表"), "\n1. ", QStringLiteral("列表项"), "\n");
    snippet(formatMenu, QStringLiteral("任务列表"), "\n- [ ] ", QStringLiteral("任务"), "\n");
    snippet(insertMenu, QStringLiteral("行内代码"), "`", "code", "`");
    snippet(insertMenu, QStringLiteral("围栏代码块"), "\n```\n", "code", "\n```\n");
    snippet(insertMenu, QStringLiteral("行内公式"), "$", "x^2", "$");
    snippet(insertMenu, QStringLiteral("独立公式"), "\n\n$$\n", "\\frac{a}{b}", "\n$$\n\n");
    snippet(insertMenu, QStringLiteral("目录"), "\n\n", "[toc]", "\n\n");
    snippet(insertMenu, QStringLiteral("分隔线"), "\n\n", "---", "\n\n");
    snippet(insertMenu, QStringLiteral("脚注"), {}, "[^1]", "\n\n[^1]: ");
    const auto table = QStringLiteral("| 列一 | 列二 |\n| --- | --- |\n| 内容 | 内容 |");
    snippet(insertMenu, QStringLiteral("表格"), "\n\n", table, "\n\n");
    insertMenu->addAction(QStringLiteral("宽表横向拖动"), this, [this, table] {
        auto text = selection();
        const QString marker = "{table:scroll}";
        if (text.contains(marker)) {
            text.remove(marker + "\n\n");
            text.remove(marker);
            insertMarkup({}, text);
        } else {
            insertMarkup("\n\n" + marker + "\n\n", text.trimmed().isEmpty() ? table : text, "\n\n");
        }
    });
    insertMenu->addAction(QStringLiteral("链接 / PDF / 媒体"), this, [this] {
        if (locked_) return;
        bool accepted = false;
        const auto entered = QInputDialog::getText(this, QStringLiteral("插入链接或媒体"), QStringLiteral("网页或文件 URL"),
            QLineEdit::Normal, {}, &accepted).trimmed();
        if (!accepted || entered.isEmpty()) return;
        const auto link = CommunityWebView::home().resolved(QUrl(entered));
        if (!link.isValid() || !link.userInfo().isEmpty() || (link.scheme() != "https" && link.scheme() != "http")) {
            setStatus(QStringLiteral("请输入有效的 HTTP 或 HTTPS 地址。"));
            return;
        }
        static const QRegularExpression embedded(QStringLiteral("^/embedded/[a-z0-9-]+\\.html$"));
        if (CommunityWebView::isSite(link) && embedded.match(link.path()).hasMatch() && link.query().isEmpty()) {
            insertMarkup("\n\n{embed-page:", link.path(), "}\n\n");
        } else {
            auto text = selection();
            if (text.isEmpty()) text = link.fileName().isEmpty() ? QStringLiteral("链接") : link.fileName();
            text.replace("]", "\\]");
            insertMarkup("[", text, QStringLiteral("](<%1>)").arg(link.toString(QUrl::FullyEncoded)));
        }
    });
    auto *color = action(QStringLiteral("颜色"), QStringLiteral("为所选文字设置颜色"), [this] { insertColor(); });
    color->setObjectName("communityInsertColor");
    auto *blackout = action(QStringLiteral("黑幕"), QStringLiteral("悬停或聚焦后显示隐藏文字"), [this] { insertBlackout(); });
    blackout->setObjectName("communityInsertBlackout");
    action(QStringLiteral("图片"), QStringLiteral("上传图片，也可在编辑区粘贴截图"), [this] {
        if (locked_ || uploading_) return;
        const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("选择论坛图片"), {},
            QStringLiteral("图片 (*.png *.jpg *.jpeg *.webp *.gif *.avif)"));
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly) || file.size() > imageLimit) {
            setStatus(QStringLiteral("图片无法读取或超过 10 MiB。"));
            return;
        }
        uploadImage(QFileInfo(path).fileName(), file.read(imageLimit + 1));
    });
    action(QStringLiteral("导入"), QStringLiteral("将 UTF-8 Markdown 插入光标处"), [this] {
        if (locked_) return;
        const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("导入 Markdown"), {},
            QStringLiteral("Markdown (*.md *.markdown *.txt)"));
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 4 * 1024 * 1024) {
            setStatus(QStringLiteral("无法读取该 Markdown 文件。"));
            return;
        }
        source_->insertPlainText(QString::fromUtf8(file.readAll()));
    });
    row->addStretch();

    auto *captions = new QHBoxLayout;
    auto *sourceTitle = new QLabel(basic_);
    captions->addWidget(sourceTitle, 1);
    captions->addWidget(new QLabel(QStringLiteral("实时预览"), basic_), 1);
    layout->addLayout(captions);
    auto *split = new QSplitter(Qt::Horizontal, basic_);
    split->setObjectName("communityLivePreviewSplit");
    split->setChildrenCollapsible(false);
    layout->addWidget(split, 1);
    auto *sourcePane = new QWidget(split);
    auto *sourceLayout = new QVBoxLayout(sourcePane);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    auto *source = new MarkdownSource(sourcePane);
    sourceLayout->addWidget(source, 1);
    source_ = source;
    source_->setObjectName("communityMarkdownSource");
    source_->setPlaceholderText(QStringLiteral("输入 Markdown，支持表格、代码块和 LaTeX 公式"));
    source->pasteImage = [this](const QString &name, const QByteArray &bytes) { uploadImage(name, bytes); };
    auto *previewPane = new QWidget(split);
    auto *previewLayout = new QVBoxLayout(previewPane);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    preview_ = new CommunityWebView(CommunityWebView::DocumentLink([](const QUrl &url) {
        QDesktopServices::openUrl(url);
    }), previewPane);
    previewLayout->addWidget(preview_, 1);
    split->setSizes({500, 500});
    status_ = new QLabel(this);
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    status_->hide();
    outer->addWidget(status_);
    refresh_ = new QTimer(this);
    refresh_->setSingleShot(true);
    refresh_->setInterval(120);
    connect(refresh_, &QTimer::timeout, this, &CommunityMarkdownEditor::preview);
    const auto updateTitle = [this, sourceTitle] {
        sourceTitle->setText(QStringLiteral("Markdown 源码 · %1 字符").arg(source_->document()->characterCount() - 1));
        if (isVisible()) refresh_->start();
    };
    connect(source_, &QPlainTextEdit::textChanged, this, updateTitle);
    connect(source_, &QPlainTextEdit::textChanged, this, [this] { if (changed_) changed_(); });
    updateTitle();
}

QString CommunityMarkdownEditor::selection() const
{
    auto text = source_->textCursor().selectedText();
    return text.replace(QChar::ParagraphSeparator, '\n');
}

void CommunityMarkdownEditor::insertMarkup(const QString &prefix, const QString &text, const QString &suffix)
{
    if (locked_) return;
    auto cursor = source_->textCursor();
    const auto start = cursor.selectionStart() + prefix.size();
    cursor.insertText(prefix + text + suffix);
    cursor.setPosition(start);
    cursor.setPosition(start + text.size(), QTextCursor::KeepAnchor);
    source_->setTextCursor(cursor);
    source_->setFocus();
}

void CommunityMarkdownEditor::insertColor()
{
    if (locked_) return;
    QColorDialog dialog(QColor("#b42336"), this);
    dialog.setObjectName("communityTextColorDialog");
    dialog.setWindowTitle(QStringLiteral("文字颜色"));
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    if (dialog.exec() != QDialog::Accepted) return;
    const auto selected = selection();
    const auto marker = QStringLiteral("{color:%1}").arg(dialog.selectedColor().name());
    if (!selected.contains('\n')) {
        insertMarkup(marker, selected.isEmpty() ? QStringLiteral("彩色文字") : selected, "{/color}");
        return;
    }
    auto lines = selected.split('\n');
    static const QRegularExpression prefix(QStringLiteral("^\\s*(?:(?:#{1,6}|>|[-+*]|[0-9]+\\.)\\s+)?"));
    for (auto &line : lines) {
        const auto offset = prefix.match(line).capturedLength();
        if (!line.mid(offset).trimmed().isEmpty()) {
            line.insert(offset, marker);
            line += "{/color}";
        }
    }
    insertMarkup({}, lines.join('\n'));
}

void CommunityMarkdownEditor::insertBlackout()
{
    if (locked_) return;
    QDialog dialog(this);
    dialog.setObjectName("communityBlackoutDialog");
    dialog.setWindowTitle(QStringLiteral("插入黑幕"));
    QFormLayout layout(&dialog);
    auto *hidden = new QPlainTextEdit(selection(), &dialog);
    hidden->setObjectName("communityBlackoutText");
    hidden->setMaximumHeight(110);
    layout.addRow(QStringLiteral("隐藏文字"), hidden);
    auto *hint = new QLineEdit(QStringLiteral("你知道的太多了"), &dialog);
    hint->setObjectName("communityBlackoutHint");
    hint->setMaxLength(120);
    layout.addRow(QStringLiteral("悬停提示"), hint);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    static const QRegularExpression reserved(QStringLiteral("[{}\\r\\n]+"));
    const auto text = hidden->toPlainText().replace(QRegularExpression(QStringLiteral("[\\r\\n]+")), " ").trimmed();
    auto tip = hint->text().replace(reserved, " ").trimmed();
    if (tip.isEmpty()) tip = QStringLiteral("隐藏文字");
    if (!text.isEmpty()) insertMarkup(QStringLiteral("{blackout:%1}").arg(tip), text, "{/blackout}");
}

QString CommunityMarkdownEditor::markdown() const { return richReady_ ? richText_ : source_->toPlainText(); }

void CommunityMarkdownEditor::setMarkdown(const QString &value)
{
    if (richReady_) {
        richText_ = value;
        rich_->editorCommand({{"type", "load"}, {"markdown", value}, {"revision", ++revision_}});
        if (changed_) changed_();
    } else if (source_->toPlainText() != value) source_->setPlainText(value);
}

void CommunityMarkdownEditor::onChange(std::function<void()> changed)
{
    changed_ = std::move(changed);
}

void CommunityMarkdownEditor::setStatus(const QString &message)
{
    status_->setText(message);
    status_->setVisible(!message.isEmpty());
}

void CommunityMarkdownEditor::setLocked(bool locked)
{
    locked_ = locked;
    source_->setReadOnly(locked);
    if (richReady_) rich_->editorCommand({{"type", "lock"}, {"locked", locked}});
}

void CommunityMarkdownEditor::uploadImage(const QString &name, const QByteArray &bytes)
{
    if (locked_ || uploading_) return;
    if (bytes.isEmpty() || bytes.size() > imageLimit) {
        setStatus(QStringLiteral("图片大小须在 10 MiB 以内。"));
        return;
    }
    uploading_ = true;
    setStatus(QStringLiteral("正在上传图片…"));
    const QPointer<CommunityMarkdownEditor> guard(this);
    auto cursor = source_->textCursor();
    upload_(name, bytes, [guard, cursor](const QUrl &url, const QString &error) mutable {
        if (!guard) return;
        guard->uploading_ = false;
        guard->setStatus(error);
        if (error.isEmpty()) {
            cursor.insertText(QStringLiteral("![图片](<%1>)").arg(url.toString(QUrl::FullyEncoded)));
            guard->source_->setTextCursor(cursor);
        }
    });
}

void CommunityMarkdownEditor::preview()
{
    if (!isVisible() || views_->currentWidget() != basic_) return;
    preview_->showDocument({
        {"blocks", QJsonArray{QJsonObject{{"markdown", markdown()}}}},
        {"base", CommunityWebView::home().toString()},
        {"key", "preview"},
        {"dark", vsr::rendererSettings().value("appearance/dark", false).toBool()}
    });
}

void CommunityMarkdownEditor::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!rich_) {
        rich_ = new CommunityWebView(CommunityWebView::DocumentLink([](const QUrl &url) {
            QDesktopServices::openUrl(url);
        }), views_);
        rich_->setObjectName("communityRichEditor");
        views_->addWidget(rich_);
        views_->setCurrentWidget(rich_);
        const auto activate = [this](bool ready) {
            if (ready == richReady_ && views_->currentWidget() == basic_) return;
            const auto previous = markdown();
            richReady_ = ready;
            if (ready) {
                rich_->editorCommand({{"type", "theme"}, {"dark", vsr::rendererSettings().value("appearance/dark", false).toBool()}});
                rich_->editorCommand({{"type", "lock"}, {"locked", locked_}});
                setMarkdown(previous);
                views_->setCurrentWidget(rich_);
            } else {
                setMarkdown(previous);
                views_->setCurrentWidget(basic_);
                refresh_->start(0);
            }
        };
        rich_->openEditor(activate, [this](const QJsonObject &message) {
            if (!richReady_) return;
            const auto type = message.value("type").toString();
            if (type == "input" && message.value("revision").toInt() == revision_) {
                const auto value = message.value("markdown").toString();
                if (value.size() > 4 * 1024 * 1024 || value == richText_) return;
                richText_ = value;
                if (changed_) changed_();
            } else if (type == "upload") {
                const auto bytes = QByteArray::fromBase64(message.value("data").toString().toLatin1(),
                    QByteArray::AbortOnBase64DecodingErrors);
                if (locked_ || uploading_ || bytes.isEmpty() || bytes.size() > imageLimit) {
                    rich_->editorCommand({{"type", "uploaded"}, {"error", QStringLiteral("当前无法上传该图片。")}});
                    return;
                }
                uploading_ = true;
                setStatus(QStringLiteral("正在上传图片…"));
                const QPointer<CommunityMarkdownEditor> guard(this);
                upload_(QFileInfo(message.value("name").toString()).fileName().left(255), bytes,
                    [guard](const QUrl &url, const QString &error) {
                        if (!guard) return;
                        guard->uploading_ = false;
                        guard->setStatus(error);
                        guard->rich_->editorCommand({{"type", "uploaded"}, {"url", url.toString(QUrl::FullyEncoded)}, {"error", error}});
                    });
            }
        });
        QTimer::singleShot(6000, this, [this, activate] { if (!richReady_) activate(false); });
    }
    refresh_->start(0);
}

void CommunityMarkdownEditor::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (preview_ && event->type() == QEvent::StyleChange) {
        const auto dark = vsr::rendererSettings().value("appearance/dark", false).toBool();
        preview_->documentTheme(dark);
        if (richReady_) rich_->editorCommand({{"type", "theme"}, {"dark", dark}});
    }
}

}
