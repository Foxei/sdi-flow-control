#include "FloatingBar.h"

#include "view/ColorDefinitions.h"

#include <QtGui/QPainter>
#include <QtWidgets/QFrame>

FloatingBar::FloatingBar(QWidget *parent) : QWidget(parent) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(4, 4, 4, 4);
  layout_->setSpacing(2);

  QColor hover = Color::TEXT;
  hover.setAlpha(18);
  setStyleSheet(QString(
      "QToolButton { border: none; border-radius: 6px; padding: 5px 8px; color: %1; }"
      "QToolButton:hover { background: %2; color: %3; }"
      "QToolButton:checked { background: %4; color: white; }")
      .arg(Color::TEXT_SECONDARY.name(), hover.name(QColor::HexArgb), Color::TEXT.name(), Color::ACCENT.name()));
}

QToolButton *FloatingBar::addButton(const QIcon &icon, const QString &text, const QString &tool_tip, bool checkable) {
  auto *button = new QToolButton(this);
  button->setIcon(icon);
  button->setIconSize(QSize(18, 18));
  button->setText(text);
  button->setToolTip(tool_tip);
  button->setCheckable(checkable);
  button->setFocusPolicy(Qt::NoFocus);
  button->setCursor(Qt::PointingHandCursor);
  button->setToolButtonStyle(text.isEmpty() ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
  layout_->addWidget(button);
  return button;
}

void FloatingBar::addSeparator() {
  auto *line = new QFrame(this);
  line->setFixedSize(1, 20);
  QColor color = Color::BASE;
  line->setStyleSheet(QString("background: %1;").arg(color.name()));
  layout_->addSpacing(4);
  layout_->addWidget(line, 0, Qt::AlignVCenter);
  layout_->addSpacing(4);
}

void FloatingBar::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  QColor fill = Color::BACKGROUND;
  fill.setAlpha(225);
  painter.setPen(QPen(Color::BASE, 1));
  painter.setBrush(fill);
  painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 9, 9);
}
