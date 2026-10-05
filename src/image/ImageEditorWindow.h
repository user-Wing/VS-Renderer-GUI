#pragma once
#include <QMainWindow>
#include <QImage>
#include <QHash>
#include <QFont>
#include <QUuid>

class QTreeWidget;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QLabel;
class QToolBar;
class QSpinBox;
class QLineEdit;
class QFontComboBox;
class QTabBar;
class QTreeWidgetItem;
class QStackedWidget;
namespace vsr {
class ImageDocument;
class ImageEditorTools;
class ImageEditorCanvas;
class ImageEditorWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit ImageEditorWindow(const QImage &image,const QString &sourcePath,QWidget *parent = nullptr);
    explicit ImageEditorWindow(ImageDocument *document,const QString &sourcePath,QWidget *parent = nullptr);
    ~ImageEditorWindow() override;
    ImageDocument *document() const { return document_; }
    ImageEditorTools *tools() const { return tools_; }
    ImageEditorCanvas *canvas() const { return canvas_; }
    void addDocument(ImageDocument *document,const QString &sourcePath);
    void addImage(const QImage &image,const QString &sourcePath);
    int documentCount() const { return pages_.size(); }
    void activateDocument(int index);
protected:
    void closeEvent(QCloseEvent *) override;
private:
    void buildUi();
    void rebuildLayers();
    void updateStatus();
    void updateColorButton();
    void savePng();
    void exportDocument(bool floating);
    void setEditorTool(int tool);
    void adjustImage(int kind);
    void addLayer();
    void deleteLayer();
    void renameLayer();
    void editTextLayer(const QUuid &id);
    void reorderLayer(int direction);
    void duplicateLayer();
    void mergeLayerDown();
    void transformLayerValues();
    void updateProperties();
    void selectAll();
    void openImage();
    void newImage();
    QWidget *makePage(ImageEditorCanvas *canvas);
    void connectDocument(ImageDocument *document,ImageEditorTools *tools,ImageEditorCanvas *canvas);
    void savePageSettings();
    void closeDocument(int index);
    void addGroup(bool fromLayer);
    void mergeAll(bool flatten);
    void exportLayer();
    void blendingOptions();
    bool isSourcePath(const QString &path) const;
    struct Page {
        ImageDocument *document;
        ImageEditorTools *tools;
        ImageEditorCanvas *canvas;
        QString path;
        bool maskPreview = false;
        int selectionMode = 0, cropRatio = 0;
        QString text;
        QFont font;
        int fontSize = 32;
        double textLineSpacing = 1.2;
        double cropWidth = 16, cropHeight = 9;
    };
    QList<Page> pages_;
    QStackedWidget *pagesStack_;
    int activePage_ = 0;
    ImageDocument *document_;
    ImageEditorTools *tools_;
    ImageEditorCanvas *canvas_;
    QTreeWidget *layers_;
    QDoubleSpinBox *layerOpacity_;
    QComboBox *blend_;
    QPushButton *color_;
    QLabel *properties_;
    QLabel *toolName_;
    QWidget *brushOptions_;
    QWidget *selectionOptions_;
    QWidget *textOptions_;
    QWidget *cropOptions_;
    QWidget *moveOptions_;
    QLineEdit *text_;
    QFontComboBox *font_;
    QSpinBox *fontSize_;
    QTabBar *documentTab_;
    QToolBar *toolBar_;
    QSpinBox *layerX_;
    QSpinBox *layerY_;
    QSpinBox *layerWidth_;
    QSpinBox *layerHeight_;
    QLabel *cursorPosition_;
    QHash<QUuid,QTreeWidgetItem *> layerItems_;
    QList<QPair<QUuid,QUuid>> layerStructure_;
    QColor backgroundColor_ = Qt::white;
    QString sourcePath_;
    QString lastPreviewError_;
};
}
