#pragma once
#include <QString>

namespace cap {
bool hasUiAccess();
bool restartWithUiAccess(QString *error);
}
