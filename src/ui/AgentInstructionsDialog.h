#pragma once

#include <QDialog>

namespace vsr {
class AgentInstructions;

class AgentInstructionsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit AgentInstructionsDialog(AgentInstructions *instructions, QWidget *parent = nullptr);
};
}
