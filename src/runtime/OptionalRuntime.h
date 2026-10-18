#pragma once
#include <QString>
namespace vsr {
// Returns a loose installation or a content-addressed, lazily extracted tool.
QString optionalRuntimeExecutable(const QString &component, const QString &program,
                                  bool extract = true, QString *error = nullptr);
}
