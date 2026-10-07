#pragma once
#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringList>
#include <QTimer>

namespace vsr {
// The native editor owns its documents and outlives the player when work is unsaved.
class PhotoCraftEditor final : public QObject {
    Q_OBJECT
public:
    static PhotoCraftEditor *instance();
    void open(const QString &path);
    static QString executablePath();
signals:
    void errorOccurred(const QString &error);
private:
    explicit PhotoCraftEditor(QObject *parent);
    void flush();
    QLocalServer server_;
    QLocalSocket *socket_ = nullptr;
    QStringList pending_;
    bool starting_ = false;
    QTimer startupTimer_;
};
}
