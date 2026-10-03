#pragma once
#include <QImage>
#include <QWidget>
#include <memory>

namespace vsr {
// Native child surface. The owning canvas keeps its SDR renderer and switches only after
// setHdrEnabled() succeeds. Mouse input passes through to that canvas.
class ImageHdrSurface final : public QWidget {
public:
    explicit ImageHdrSurface(QWidget *parent = nullptr);
    ~ImageHdrSurface() override;
    bool setHdrEnabled(bool enabled);
    bool hdrActive() const;
    QString status() const;
    static bool validateShaders(QString *error = nullptr); // Runtime compiler smoke check; no HDR display required.
    void present(const QImage &native, const QRectF &target, const QImage &overlay,
                 double exposure = 0, double paperWhiteNits = 203);
    QPaintEngine *paintEngine() const override;
protected:
    void paintEvent(QPaintEvent *) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};
}
