#pragma once
#include "agent/AgentSession.h"
#include <QObject>

namespace vsr {
// Isolated VS requests leave the visible preview, playback and filter graph intact.
class AgentVideoFrames final : public QObject {
public:
    using Script = std::function<QString(bool processed, QString *error)>;
    AgentVideoFrames(AgentSession *session, Script script);
};
}
