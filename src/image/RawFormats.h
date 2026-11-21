#pragma once
#include "image/RawExtensions.h"
#include <QStringList>
namespace vsr {
inline QStringList playerRawExtensions(){QStringList extensions;for(const auto *extension:rawExtensions)extensions.append(QString::fromLatin1(extension));return extensions;}
}
