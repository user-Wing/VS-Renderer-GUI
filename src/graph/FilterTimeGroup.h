#pragma once
#include "graph/FilterGraph.h"

namespace vsr {
// Ordered boundaries in whole source seconds; the last group runs to EOF.
struct FilterTimeGroup {
    QString name;
    qint64 startSeconds = 0;
    FilterGraph graph;
    QString script; // Script-backed single-group presets (including player profiles).
    qint64 startFrame = -1; // >=0: exact zero-based SOURCE frame, otherwise legacy whole seconds.
};
}
