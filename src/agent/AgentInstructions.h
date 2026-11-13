#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

namespace vsr {

class AgentInstructions final : public QObject {
    Q_OBJECT
public:
    explicit AgentInstructions(const QString &rootDirectory = {}, QObject *parent = nullptr);
    QString rootDirectory() const { return root_; }
    QString globalPromptPath() const;
    QString loadGlobalPrompt(QString *error = nullptr) const;
    bool saveGlobalPrompt(const QString &text, QString *error = nullptr);
    bool refresh(QString *error = nullptr);
    QString systemPrompt() const;
    QJsonArray skills() const { return skills_; }
    bool importSkill(const QString &path, QString *error = nullptr);
    bool removeSkill(const QString &name, QString *error = nullptr);
    bool setSkillEnabled(const QString &name, bool enabled, QString *error = nullptr);
    QJsonObject readSkill(const QString &name, const QString &relativePath = QStringLiteral("SKILL.md")) const;
signals:
    void changed();
private:
    bool saveEnabled(const QJsonObject &enabled, QString *error);
    QString root_, globalPrompt_;
    QJsonArray skills_;
    QJsonObject enabled_;
};

}
