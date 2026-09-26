#pragma once
#include <QListWidget>

namespace vsr {

class PreparedFiles final : public QListWidget {
    Q_OBJECT
public:
    explicit PreparedFiles(QWidget *parent = nullptr);
    void addFiles(const QStringList &paths);
    QStringList files() const;
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

}
