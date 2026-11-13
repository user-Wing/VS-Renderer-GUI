#pragma once
#include "agent/AgentProvider.h"
#include <QDialog>
namespace vsr {
class AgentTaskQueue;
class AgentSession;
class AgentTaskDialog final : public QDialog {
public:
    AgentTaskDialog(AgentTaskQueue *queue, AgentSession *parentSession, QWidget *parent = nullptr);
};
}
