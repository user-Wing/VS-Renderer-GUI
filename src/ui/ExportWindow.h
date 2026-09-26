#pragma once

#include "backend/ExportPipeline.h"
#include "graph/VpyScriptBuilder.h"
#include <QElapsedTimer>
#include <QHash>
#include <QWidget>
#include <functional>
#include <memory>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QTemporaryDir;

namespace vsr {
class PreparedFiles;

class ExportWindow final : public QWidget {
    Q_OBJECT
public:
    explicit ExportWindow();
    void setCurrentSource(const QString &source);
    void setScriptBuilder(std::function<ScriptBuildResult(const QString &)> builder);
    bool addJob(const QString &command, const QString &output, const QString &source,
                const QString &script, double durationSeconds, QString *error = nullptr);
    bool isBusy() const;
    void stopAll();
    static QString outputPath(const QString &source, const QString &directory, bool timestamp);
signals:
    void statusMessage(const QString &message);
    void idle();
private:
    enum class State { Waiting, Running, Paused, Stopping, Stopped, Completed, Failed };
    struct Job {
        ExportPlan plan;
        QString source;
        QString output;
        QString sourceKey;
        QString outputKey;
        std::shared_ptr<QTemporaryDir> directory;
        QTreeWidgetItem *item = nullptr;
        double durationSeconds = 0;
        State state = State::Waiting;
        bool started = false;
    };
    void enqueue(bool batch);
    void startNext();
    void stopJob(const std::shared_ptr<Job> &job);
    void togglePause();
    void removeWaiting();
    void restartStopped();
    std::shared_ptr<Job> jobFor(QTreeWidgetItem *item) const;
    ExportPipeline pipeline_;
    QHash<quint64, std::shared_ptr<Job>> jobs_;
    std::shared_ptr<Job> active_;
    quint64 nextId_ = 0;
    QTabWidget *tabs_ = nullptr;
    QTreeWidget *table_ = nullptr;
    PreparedFiles *prepared_ = nullptr;
    QPlainTextEdit *command_ = nullptr;
    QPlainTextEdit *log_ = nullptr;
    QLineEdit *singleOutput_ = nullptr;
    QLabel *sourceLabel_ = nullptr;
    QLabel *message_ = nullptr;
    QComboBox *destination_ = nullptr;
    QLineEdit *directory_ = nullptr;
    QComboBox *naming_ = nullptr;
    QPushButton *pause_ = nullptr;
    QElapsedTimer elapsed_;
    QElapsedTimer pauseTime_;
    qint64 pausedMilliseconds_ = 0;
    bool queuePaused_ = false;
    QString currentSource_;
    std::function<ScriptBuildResult(const QString &)> scriptBuilder_;
};

}
