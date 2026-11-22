#include "StatusBar.h"

#include "view/SizeDefinitions.h"

#include <cmath>

StatusBar::StatusBar(QWidget *parent) : QStatusBar(parent) {
  setUpGui();
  loadImages();
  showReady();
}

void StatusBar::setUpGui() {
  setContentsMargins(Size::STATUS_PADDING, Size::STATUS_PADDING / 2, Size::STATUS_PADDING, Size::STATUS_PADDING / 2);
  setMinimumHeight(Size::STATUS_HEIGHT);
  setSizeGripEnabled(false);

  label_img = new QLabel(this);

  // Long messages may be cut off instead of widening the window
  label_message = new QLabel(this);
  label_message->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  // Tabular figures keep the values from jumping while they change
  QFont font_values = font();
  font_values.setFeature(QFont::Tag("tnum"), 1);
  label_cell = new QLabel(this);
  label_values = new QLabel(this);
  label_rate = new QLabel(this);
  for (QLabel *label : {label_cell, label_values, label_rate}) {
    label->setFont(font_values);
    label->setForegroundRole(QPalette::PlaceholderText);
    label->setContentsMargins(Size::STATUS_PADDING, 0, 0, 0);
  }
  label_cell->setToolTip(tr("Cell under the mouse"));
  label_values->setToolTip(tr("Speed and pressure of the cell under the mouse"));
  label_rate->setToolTip(tr("Solver steps per second"));

  addWidget(label_img);
  addWidget(label_message, 1);
  addPermanentWidget(label_cell);
  addPermanentWidget(label_values);
  addPermanentWidget(label_rate);

  timer_reset_ = new QTimer(this);
  timer_reset_->setSingleShot(true);
  connect(timer_reset_, &QTimer::timeout, this, &StatusBar::showReady);

  displayCell(QPoint(), 0, 0);
}

void StatusBar::displayMessage(const QString &message, MessageTyp typ, int timeout) {
  label_message->setText(message);
  label_message->setToolTip(message);
  QPixmap img;
  switch (typ) {
    case MessageTyp::Error: img = icons_[0].pixmap(16); break;
    case MessageTyp::OK: img = icons_[1].pixmap(16); break;
    case MessageTyp::NONE: break;
  }
  label_img->setPixmap(img);
  label_img->setVisible(!img.isNull());
  if (timeout > 0) {
    timer_reset_->start(timeout);
  } else {
    timer_reset_->stop();
  }
}

void StatusBar::showReady() {
  displayMessage(tr("Ready"), MessageTyp::OK);
}

void StatusBar::displayCell(QPoint cell, double speed, double pressure) {
  bool visible = !cell.isNull();
  label_cell->setVisible(visible);
  label_values->setVisible(visible);
  if (!visible) {
    return;
  }
  label_cell->setText(tr("Cell %1, %2").arg(cell.x()).arg(cell.y()));
  label_values->setText(tr("Speed %1  Pressure %2")
                            .arg(speed, 0, 'f', 2)
                            .arg(pressure, 0, 'f', 2));
}

void StatusBar::displayRate(double rate) {
  label_rate->setText(tr("%1 steps/s").arg(std::lround(rate)));
}

void StatusBar::loadImages() {
  icons_[0] = QIcon(":icons/error.svg");
  icons_[1] = QIcon(":icons/ok.svg");
}
