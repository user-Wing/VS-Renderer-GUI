#include "app/Style.h"

namespace vsr {

QString applicationStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QWidget { background: #f3f3f3; color: #1b1b1b; }
        QToolBar { background: #f9f9f9; border: 0; border-bottom: 1px solid #d1d1d1; spacing: 4px; padding: 5px 8px; }
        QToolButton, QPushButton, QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit {
            min-height: 28px; border: 1px solid #c8c8c8; border-radius: 3px; background: #ffffff; padding: 0 8px;
        }
        QToolButton:hover, QPushButton:hover, QComboBox:hover { background: #f0f0f0; border-color: #a8a8a8; }
        QToolButton:pressed, QPushButton:pressed { background: #e5e5e5; }
        QToolButton:disabled, QPushButton:disabled { color: #929292; background: #eeeeee; }
        QToolButton:checked, QPushButton:checked { background: #e5f1fb; border-color: #5aa7df; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-bottom: 2px solid #0067c0; }
        QListWidget, QTreeWidget, QPlainTextEdit, QScrollArea { background: #ffffff; border: 1px solid #d1d1d1; }
        QListWidget::item { min-height: 32px; padding: 1px 6px; }
        QListWidget::item:selected { color: #111111; background: #d9ebf7; border-left: 3px solid #0067c0; }
        QSplitter::handle { background: #d1d1d1; }
        QSlider::groove:horizontal { height: 4px; background: #c6c6c6; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #0067c0; border-radius: 2px; }
        QSlider::handle:horizontal { width: 12px; margin: -5px 0; border-radius: 6px; background: #0067c0; }
        QCheckBox[vrrToggle="true"] { border: 1px solid #a8a8a8; border-radius: 3px; padding: 5px; background: #ffffff; }
        QCheckBox[vrrToggle="true"]:checked { border-color: #0067c0; background: #d9ebf7; }
        QCheckBox[vrrToggle="true"]:hover { border-color: #0067c0; background: #e5f1fb; }
        QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #727272; border-radius: 2px; background: #ffffff; }
        QCheckBox::indicator:checked { background: #0067c0; border-color: #0067c0; image: url(:/ui/check.svg); }
        QMenu::item:selected, QComboBox QAbstractItemView::item:selected, QComboBox QAbstractItemView::item:hover { background: #d9ebf7; color: #111111; }
        QStatusBar { background: #f9f9f9; border-top: 1px solid #d1d1d1; }
        QToolTip { color: #111111; background: #ffffff; border: 1px solid #8a8a8a; padding: 4px; }
    )");
}

}
