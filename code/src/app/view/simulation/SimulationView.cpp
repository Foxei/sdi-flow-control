#include "SimulationView.h"

#include "view/ColorDefinitions.h"

#include <QtGui/QLinearGradient>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

constexpr double MARGIN_SIDE = 16;   // space left and right of the channel
constexpr double MARGIN_TOP = 64;    // space for the legend
constexpr double MARGIN_BOTTOM = 72; // space for the field selection

/**
 * Linear interpolation between two colors.
 */
QColor mix(const QColor &a, const QColor &b, double t) {
  t = std::clamp(t, 0.0, 1.0);
  return QColor::fromRgbF(static_cast<float>(a.redF() + (b.redF() - a.redF()) * t),
                          static_cast<float>(a.greenF() + (b.greenF() - a.greenF()) * t),
                          static_cast<float>(a.blueF() + (b.blueF() - a.blueF()) * t));
}

/**
 * Speed color: slow is dim, fast is bright. t in [0, 1]
 */
QColor speedColor(double t) {
  t = std::clamp(t, 0.0, 1.0);
  return t < 0.5 ? mix(Color::SPEED_LOW, Color::SPEED_MID, t * 2)
                 : mix(Color::SPEED_MID, Color::SPEED_HIGH, t * 2 - 1);
}

/**
 * Signed field color over the background. t in [-1, 1]
 */
QColor signedColor(double t) {
  return t < 0 ? mix(Color::SURFACE, Color::FIELD_NEGATIVE, -t)
               : mix(Color::SURFACE, Color::FIELD_POSITIVE, t);
}

/**
 * Ink color, the square root makes thin ink visible. t in [0, 1]
 */
QColor inkColor(double t) {
  return mix(Color::SURFACE, Color::INK, std::sqrt(std::clamp(t, 0.0, 1.0)));
}

}

SimulationView::SimulationView(QWidget *parent) : QWidget(parent) {
  setUpGui();
}

void SimulationView::setUpGui() {
  setMouseTracking(true);
  setMinimumSize(420, 320);
  setAttribute(Qt::WA_OpaquePaintEvent);

  // Field selection (keys 1 to 4) and clear button
  bar_views_ = new FloatingBar(this);
  group_views_ = new QButtonGroup(this);
  group_views_->setExclusive(true);
  struct View {
    VisualisationMode mode;
    const char *icon;
    QString text;
    QString tool_tip;
  };
  const std::array<View, 4> views = {{
      {VisualisationMode::Divergence, ":icons/divergence.svg", tr("Divergence"), tr("Show the divergence of the velocity")},
      {VisualisationMode::Pressure, ":icons/pressure.svg", tr("Pressure"), tr("Show the pressure field")},
      {VisualisationMode::Velocity, ":icons/velocity.svg", tr("Velocity"), tr("Show the velocity field")},
      {VisualisationMode::Densities, ":icons/density.svg", tr("Density"), tr("Show the ink density")},
  }};
  int key = 1;
  for (const View &view : views) {
    QKeySequence shortcut(Qt::Key_0 + key);
    QToolButton *button = bar_views_->addButton(
        QIcon(view.icon), view.text,
        QString("%1 (%2)").arg(view.tool_tip, shortcut.toString(QKeySequence::NativeText)), true);
    button->setShortcut(shortcut);
    button->setChecked(view.mode == mode_);
    group_views_->addButton(button, static_cast<int>(view.mode));
    key++;
  }
  bar_views_->addSeparator();
  QToolButton *button_clear = bar_views_->addButton(
      QIcon(":icons/clear.svg"), QString(), QString());
  button_clear->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
  button_clear->setToolTip(tr("Remove all particles and traces (%1)")
                               .arg(button_clear->shortcut().toString(QKeySequence::NativeText)));

  // Sidebar toggle
  bar_sidebar_ = new FloatingBar(this);
  button_sidebar_ = bar_sidebar_->addButton(QIcon(":icons/sidebar.svg"), QString(), QString(), true);
  button_sidebar_->setChecked(true);
  button_sidebar_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
  button_sidebar_->setToolTip(tr("Show or hide the sidebar (%1)")
                                  .arg(button_sidebar_->shortcut().toString(QKeySequence::NativeText)));

  connect(group_views_, &QButtonGroup::idClicked, this, [this](int id) {
    setMode(static_cast<VisualisationMode>(id));
  });
  connect(button_clear, &QToolButton::clicked, this, &SimulationView::clearParticlesClicked);
  connect(button_sidebar_, &QToolButton::toggled, this, &SimulationView::sidebarToggled);
}

void SimulationView::setFrame(FramePtr frame) {
  frame_ = std::move(frame);
  update();
}

void SimulationView::setMode(VisualisationMode mode) {
  if (QAbstractButton *button = group_views_->button(static_cast<int>(mode))) {
    button->setChecked(true);
  }
  if (mode == mode_) {
    return;
  }
  mode_ = mode;
  update();
  emit modeChanged(mode_);
}

void SimulationView::setSidebarVisible(bool visible) {
  QSignalBlocker blocker(button_sidebar_);
  button_sidebar_->setChecked(visible);
}

void SimulationView::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  bar_views_->adjustSize();
  bar_views_->move((width() - bar_views_->width()) / 2, height() - bar_views_->height() - 16);
  bar_sidebar_->adjustSize();
  bar_sidebar_->move(width() - bar_sidebar_->width() - 16, 16);
}

// ---------------------------------------------------------------------------
// Coordinates
// ---------------------------------------------------------------------------

double SimulationView::cellSize() const {
  if (!frame_) {
    return 1;
  }
  double available_width = width() - 2 * MARGIN_SIDE;
  double available_height = height() - MARGIN_TOP - MARGIN_BOTTOM;
  // Interior plus one wall row at the top and at the bottom
  return std::max(1.0, std::min(available_width / frame_->width, available_height / (frame_->height + 2)));
}

QRectF SimulationView::interiorRect() const {
  if (!frame_) {
    return {};
  }
  double cell = cellSize();
  QSizeF size(frame_->width * cell, frame_->height * cell);
  QPointF center(width() / 2.0, MARGIN_TOP + (height() - MARGIN_TOP - MARGIN_BOTTOM) / 2.0);
  // Align to device pixels so cell edges stay sharp
  QPointF top_left = center - QPointF(size.width() / 2, size.height() / 2);
  top_left = QPointF(std::round(top_left.x()), std::round(top_left.y()));
  return {top_left, size};
}

QPointF SimulationView::toGrid(const QPointF &position) const {
  QRectF interior = interiorRect();
  double cell = cellSize();
  return {(position.x() - interior.left()) / cell + 1, (interior.bottom() - position.y()) / cell + 1};
}

QPointF SimulationView::toWidget(double x, double y) const {
  QRectF interior = interiorRect();
  double cell = cellSize();
  return {interior.left() + (x - 1) * cell, interior.bottom() - (y - 1) * cell};
}

bool SimulationView::isInside(const QPointF &grid) const {
  return frame_ && grid.x() >= 1 && grid.x() <= frame_->width + 1
         && grid.y() >= 1 && grid.y() <= frame_->height + 1;
}

QPoint SimulationView::cellAt(const QPointF &grid) const {
  return {std::clamp(static_cast<int>(std::floor(grid.x())), 1, frame_->width),
          std::clamp(static_cast<int>(std::floor(grid.y())), 1, frame_->height)};
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void SimulationView::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.fillRect(rect(), palette().window());
  if (!frame_) {
    return;
  }
  QRectF interior = interiorRect();
  double cell = cellSize();

  // Channel walls
  painter.fillRect(QRectF(interior.left(), interior.top() - cell, interior.width(), cell), Color::WALL);
  painter.fillRect(QRectF(interior.left(), interior.bottom(), interior.width(), cell), Color::WALL);

  // Field
  if (mode_ == VisualisationMode::Velocity) {
    painter.fillRect(interior, Color::SURFACE);
  } else {
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(interior, fieldImage());
  }
  drawGrid(painter);
  if (mode_ == VisualisationMode::Velocity) {
    drawVelocity(painter);
  }
  drawObstacles(painter);
  drawParticles(painter);

  // Cell under the mouse
  if (!hover_cell_.isNull() && !force_drag_) {
    QColor outline = Color::TEXT;
    outline.setAlpha(110);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(outline, 1));
    painter.setBrush(Qt::NoBrush);
    QPointF corner = toWidget(hover_cell_.x(), hover_cell_.y() + 1);
    painter.drawRect(QRectF(corner, QSizeF(cell, cell)).adjusted(0, 0, -1, -1));
  }
  drawForce(painter);
  drawLegend(painter);
}

QImage SimulationView::fieldImage() const {
  const int w = frame_->width;
  const int h = frame_->height;
  QImage image(w, h, QImage::Format_RGB32);
  for (int j = 1; j <= h; j++) {
    auto *line = reinterpret_cast<QRgb *>(image.scanLine(h - j));
    for (int i = 1; i <= w; i++) {
      QColor color;
      switch (mode_) {
        case VisualisationMode::Pressure:
          color = signedColor(frame_->pressure(i, j) / frame_->max_pressure);
          break;
        case VisualisationMode::Divergence:
          color = signedColor(frame_->divergence(i, j) / frame_->max_divergence);
          break;
        case VisualisationMode::Densities:
          color = inkColor(frame_->density(i, j) * 2);
          break;
        case VisualisationMode::Velocity:
          color = Color::SURFACE;
          break;
      }
      line[i - 1] = color.rgb();
    }
  }
  return image;
}

void SimulationView::drawGrid(QPainter &painter) const {
  double cell = cellSize();
  if (cell < 8) {
    return;
  }
  QRectF interior = interiorRect();
  QColor color = Color::TEXT;
  color.setAlpha(10);
  QVector<QLineF> lines;
  for (int i = 1; i < frame_->width; i++) {
    double x = interior.left() + i * cell;
    lines.append(QLineF(x, interior.top(), x, interior.bottom()));
  }
  for (int j = 1; j < frame_->height; j++) {
    double y = interior.top() + j * cell;
    lines.append(QLineF(interior.left(), y, interior.right(), y));
  }
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setPen(QPen(color, 0));
  painter.drawLines(lines);
}

void SimulationView::drawVelocity(QPainter &painter) const {
  // Arrows are grouped by color to draw them in few calls
  constexpr int bins = 24;
  std::array<QVector<QLineF>, bins> lines;
  double cell = cellSize();
  double max_speed = std::max(1e-6f, frame_->max_speed);

  for (int i = 1; i <= frame_->width; i++) {
    for (int j = 1; j <= frame_->height; j++) {
      if (frame_->blocked(i, j) != 0) {
        continue;
      }
      // A line through the cell center, one cell long at the maximum speed
      QPointF velocity(frame_->velocity_x(i, j), -frame_->velocity_y(i, j));
      velocity *= cell / max_speed / 2;
      QPointF center = toWidget(i + 0.5, j + 0.5);
      double t = frame_->speed(i, j) / max_speed;
      int bin = std::clamp(static_cast<int>(t * bins), 0, bins - 1);
      lines[bin].append(QLineF(center - velocity, center + velocity));
    }
  }
  painter.setRenderHint(QPainter::Antialiasing);
  for (int bin = 0; bin < bins; bin++) {
    QPen pen(speedColor((bin + 0.5) / bins), std::clamp(cell * 0.12, 1.0, 2.5));
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLines(lines[bin]);
  }
}

void SimulationView::drawObstacles(QPainter &painter) const {
  double cell = cellSize();
  painter.setRenderHint(QPainter::Antialiasing, false);
  for (int i = 1; i <= frame_->width; i++) {
    for (int j = 1; j <= frame_->height; j++) {
      if (frame_->blocked(i, j) != 0) {
        painter.fillRect(QRectF(toWidget(i, j + 1), QSizeF(cell, cell)), Color::OBSTACLE);
      }
    }
  }
}

void SimulationView::drawParticles(QPainter &painter) const {
  double cell = cellSize();
  double radius = std::clamp(cell * 0.18, 1.5, 4.0);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.save();
  painter.setClipRect(interiorRect());

  // Traces
  QColor trace = Color::PARTICLE;
  trace.setAlpha(170);
  painter.setPen(QPen(trace, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  for (const auto *traces : {&frame_->trajectories, &frame_->finished_traces}) {
    for (const auto &trajectory : *traces) {
      QPolygonF line;
      line.reserve(static_cast<qsizetype>(trajectory.size()));
      for (const Eigen::Vector2f &point : trajectory) {
        line.append(toWidget(point(0), point(1)));
      }
      painter.drawPolyline(line);
    }
  }

  // Particles and heads of the moving traces
  painter.setPen(Qt::NoPen);
  painter.setBrush(Color::PARTICLE);
  for (const Eigen::Vector2f &point : frame_->particles) {
    painter.drawEllipse(toWidget(point(0), point(1)), radius, radius);
  }
  for (const auto &trajectory : frame_->trajectories) {
    const Eigen::Vector2f &point = trajectory.back();
    painter.drawEllipse(toWidget(point(0), point(1)), radius, radius);
  }
  painter.restore();
}

void SimulationView::drawForce(QPainter &painter) const {
  if (!force_drag_) {
    return;
  }
  QPointF start = toWidget(force_start_.x(), force_start_.y());
  QPointF end = toWidget(force_end_.x(), force_end_.y());
  QLineF line(start, end);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(Color::ACCENT.lighter(120), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  painter.drawLine(line);
  if (line.length() > 6) {
    QLineF head_left = QLineF::fromPolar(10, line.angle() + 150).translated(end);
    QLineF head_right = QLineF::fromPolar(10, line.angle() - 150).translated(end);
    painter.drawPolyline(QPolygonF({head_left.p2(), end, head_right.p2()}));
  }
  painter.setPen(Qt::NoPen);
  painter.setBrush(Color::ACCENT.lighter(120));
  painter.drawEllipse(start, 3.5, 3.5);
}

void SimulationView::drawLegend(QPainter &painter) const {
  QString title;
  QString low;
  QString high;
  QLinearGradient gradient;
  auto number = [](double value) { return QString::number(value, 'g', 2); };
  switch (mode_) {
    case VisualisationMode::Velocity:
      title = tr("Speed");
      low = number(0);
      high = number(frame_->max_speed);
      gradient.setColorAt(0, speedColor(0));
      gradient.setColorAt(0.5, speedColor(0.5));
      gradient.setColorAt(1, speedColor(1));
      break;
    case VisualisationMode::Densities:
      title = tr("Ink density");
      low = number(0);
      high = number(0.5);
      for (double t : {0.0, 0.1, 0.25, 0.5, 1.0}) {
        gradient.setColorAt(t, inkColor(t));
      }
      break;
    case VisualisationMode::Pressure:
    case VisualisationMode::Divergence: {
      double max = mode_ == VisualisationMode::Pressure ? frame_->max_pressure : frame_->max_divergence;
      title = mode_ == VisualisationMode::Pressure ? tr("Pressure") : tr("Divergence");
      low = number(-max);
      high = number(max);
      gradient.setColorAt(0, signedColor(-1));
      gradient.setColorAt(0.5, signedColor(0));
      gradient.setColorAt(1, signedColor(1));
      break;
    }
  }

  const QFontMetrics metrics(font());
  const double padding = 10;
  const double bar_width = 160;
  QRectF box(MARGIN_SIDE, 16, bar_width + 2 * padding, metrics.height() * 2 + 6 + 2 * padding + 4);

  painter.setRenderHint(QPainter::Antialiasing);
  QColor fill = Color::BACKGROUND;
  fill.setAlpha(225);
  painter.setPen(QPen(Color::BASE, 1));
  painter.setBrush(fill);
  painter.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), 9, 9);

  QRectF bar(box.left() + padding, box.top() + padding + metrics.height() + 4, bar_width, 6);
  gradient.setStart(bar.topLeft());
  gradient.setFinalStop(bar.topRight());
  painter.setPen(Qt::NoPen);
  painter.setBrush(gradient);
  painter.drawRoundedRect(bar, 3, 3);

  painter.setPen(Color::TEXT);
  painter.drawText(QRectF(bar.left(), box.top() + padding, bar_width, metrics.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, title);
  painter.setPen(Color::TEXT_SECONDARY);
  QRectF labels(bar.left(), bar.bottom() + 4, bar_width, metrics.height());
  painter.drawText(labels, Qt::AlignLeft | Qt::AlignVCenter, low);
  painter.drawText(labels, Qt::AlignRight | Qt::AlignVCenter, high);
}

// ---------------------------------------------------------------------------
// Mouse input
// ---------------------------------------------------------------------------

void SimulationView::updateHover(const QPointF &position) {
  QPointF grid = toGrid(position);
  QPoint cell = isInside(grid) ? cellAt(grid) : QPoint();
  setCursor(cell.isNull() ? Qt::ArrowCursor : Qt::CrossCursor);
  if (cell != hover_cell_) {
    hover_cell_ = cell;
    update();
    emit cellHovered(cell);
  }
}

void SimulationView::handleMouse(QMouseEvent *event) {
  QPointF grid = toGrid(event->position());
  if (!isInside(grid)) {
    return;
  }
  QPoint cell = cellAt(grid);
  Qt::MouseButtons buttons = event->buttons();
  bool shift = event->modifiers().testFlag(Qt::ShiftModifier);

  if (shift) {
    if (buttons == Qt::LeftButton) {
      emit obstacleAdded(cell);
    } else if (buttons == Qt::RightButton) {
      emit obstacleRemoved(cell);
    }
    return;
  }
  // Ink is only visible in the density view, the other views release
  // particles that leave a trace instead
  auto addInk = [this, cell] {
    if (mode_ == VisualisationMode::Densities) {
      emit inkAdded(cell);
    } else {
      emit tracerAdded(cell);
    }
  };
  if (buttons == Qt::LeftButton) {
    emit particleAdded(grid);
  } else if (buttons == Qt::RightButton) {
    addInk();
  } else if (buttons == Qt::MiddleButton) {
    emit particleAdded(grid);
    addInk();
  }
}

void SimulationView::mousePressEvent(QMouseEvent *event) {
  QPointF grid = toGrid(event->position());
  if (!isInside(grid)) {
    return;
  }
  if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::ControlModifier)) {
    force_drag_ = true;
    force_cell_ = cellAt(grid);
    force_start_ = grid;
    force_end_ = grid;
    update();
    return;
  }
  handleMouse(event);
}

void SimulationView::mouseMoveEvent(QMouseEvent *event) {
  updateHover(event->position());
  if (force_drag_) {
    force_end_ = toGrid(event->position());
    update();
  } else if (event->buttons() != Qt::NoButton) {
    handleMouse(event);
  }
}

void SimulationView::mouseReleaseEvent(QMouseEvent *event) {
  if (!force_drag_ || event->button() != Qt::LeftButton) {
    return;
  }
  force_drag_ = false;
  update();
  QPointF direction = toGrid(event->position()) - force_start_;
  double length = std::hypot(direction.x(), direction.y());
  if (length > 0.25) {
    emit forceApplied(force_cell_, direction / length);
  }
}

void SimulationView::leaveEvent(QEvent *event) {
  QWidget::leaveEvent(event);
  if (!hover_cell_.isNull()) {
    hover_cell_ = QPoint();
    update();
    emit cellHovered(QPoint());
  }
}
