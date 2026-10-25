#pragma once
#include <QString>

namespace cap {
bool hasUiAccess();
bool restartWithUiAccess(QString *error);
bool configureElevatedStartup(bool enabled, QString *error);
int runStartupTaskCommand(bool enabled, const QString &user);
}
