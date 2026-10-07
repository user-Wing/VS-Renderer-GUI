#pragma once
#include <QWidget>
#include <QImage>
#include <QTransform>
#include <QStringList>

class QThread;
namespace vsr {
class PreviewPane;

struct ImageOutput {
    QString source, destination;
    QImage image;
    QTransform transform;
    QRect crop;
    QSize size;
    QString format; // Empty uses Qt's lossless PNG writer; otherwise the AWJ backend.
    int quality = 70, speed = 5;
    bool visualQuality = false;
    QString chroma = "420", bitDepth = "auto", alpha = "auto";
    QString resizeAlgorithm = "jinc";
};

class PlayerImageTools final : public QWidget {
    Q_OBJECT
public:
    explicit PlayerImageTools(PreviewPane *pane, QWidget *parent = nullptr);
    ~PlayerImageTools() override;
    void setSource(const QString &path);
    void setReady(bool ready);
    static QString backendPath();
    static QStringList conversionArguments(const ImageOutput &output, const QString &input, const QString &directory);
    static QString writeOutput(const ImageOutput &output); // Empty string means success.
    static QString writeEditorImport(const QImage &image, const QString &path, const QString &name);
    static QImage resample(const QImage &image, const QSize &size, const QString &algorithm);
signals:
    void errorOccurred(const QString &error);
    void statusChanged(const QString &status);
    void imageRemoved(const QString &path);
private:
    ImageOutput currentOutput() const;
    void editImage();
    void convertImage();
    void recycleImage();
    void wallpaper();
    void startOutput(const ImageOutput &output, bool desktop = false);
    PreviewPane *pane_;
    QString source_;
    QThread *task_ = nullptr;
    bool ready_ = false;
};
}
