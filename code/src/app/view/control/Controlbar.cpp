#include "Controlbar.h"

#include <QtWidgets/QHBoxLayout>

#include "view/SizeDefinitions.h"

ControlBar::ControlBar(QWidget *parent) : QWidget(parent) {
  setUpGui();
}

void ControlBar::setUpGui() {
  // Secondary action
  button_clear_obstacles_ = new QPushButton(QIcon(":icons/eraser.svg"), tr("Clear"), this);
  button_clear_obstacles_->setToolTip(tr("Remove all obstacles"));
  button_clear_obstacles_->setMinimumHeight(Size::BUTTON_HEIGHT);

  // Primary action (Ctrl+R, Cmd+R on macOS)
  button_restart_ = new QPushButton(QIcon(":icons/restart.svg"), tr("Restart"), this);
  button_restart_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  button_restart_->setToolTip(tr("Restart the solver with the entered grid size (%1)")
                                  .arg(button_restart_->shortcut().toString(QKeySequence::NativeText)));
  button_restart_->setDefault(true);
  button_restart_->setMinimumHeight(Size::BUTTON_HEIGHT);
  QColor accent = palette().color(QPalette::Accent);
  button_restart_->setStyleSheet(QString(
      "QPushButton { background: %1; color: white; border: none; border-radius: 6px; font-weight: 600; }"
      "QPushButton:hover { background: %2; }"
      "QPushButton:pressed { background: %3; }"
      "QPushButton:disabled { background: %4; color: %5; }")
      .arg(accent.name(), accent.lighter(110).name(), accent.darker(115).name(),
           palette().color(QPalette::Button).name(),
           palette().color(QPalette::Disabled, QPalette::ButtonText).name()));

  auto *layout = new QHBoxLayout(this);
  layout->addWidget(button_clear_obstacles_, 1);
  layout->addWidget(button_restart_, 2);
  layout->setSpacing(Size::FORM_SPACING / 2);
  layout->setContentsMargins(12, 8, 12, 12);

  connect(button_restart_, &QPushButton::clicked, this, &ControlBar::restartClicked);
  connect(button_clear_obstacles_, &QPushButton::clicked, this, &ControlBar::clearObstaclesClicked);
}
