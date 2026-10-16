#pragma once
#include "graph/FilterGraph.h"

namespace vsr {
// Ordered boundaries in whole source seconds; the last group runs to EOF.
struct FilterTimeGroup {
    QString name;
    qint64 startSeconds = 0;
    FilterGraph graph;
};
}
