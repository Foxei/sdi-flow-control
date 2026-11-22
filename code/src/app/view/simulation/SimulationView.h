#ifndef REAL_TIME_FLUID_SIMULATION_VIEW_H
#define REAL_TIME_FLUID_SIMULATION_VIEW_H

#include "model/Definitions.h"
#include "view/simulation/FloatingBar.h"

#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

/**
 * Draws the simulation and turns mouse input into solver input. Floating
 * bars above the simulation switch the displayed field.
 *
 * Grid coordinates: cell (i, j) covers [i, i + 1) x [j, j + 1), the channel
 * spans 1 <= x <= width + 1 and 1 <= y <= height + 1, y points up.
 */
class SimulationView : public QWidget {
 Q_OBJECT
 private:
  FramePtr frame_;                                      //< Last frame of the solver
  VisualisationMode mode_ = VisualisationMode::Velocity; //< Displayed field

  FloatingBar *bar_views_ = nullptr;   //< Field selection and clear button
  FloatingBar *bar_sidebar_ = nullptr; //< Toggle of the sidebar
  QButtonGroup *group_views_ = nullptr;
  QToolButton *button_sidebar_ = nullptr;

  QPoint hover_cell_;          //< Cell under the mouse, null if outside
  bool force_drag_ = false;    //< A force is being dragged
  QPoint force_cell_;          //< Cell where the force drag started
  QPointF force_start_;        //< Start of the force drag in grid coordinates
  QPointF force_end_;          //< Current end of the force drag in grid coordinates

  void setUpGui();

  /**
   * Area of the channel interior (without walls) on the widget.
   */
  QRectF interiorRect() const;

  /**
   * Size of one cell on the widget.
   */
  double cellSize() const;

  QPointF toGrid(const QPointF &position) const;
  QPointF toWidget(double x, double y) const;
  bool isInside(const QPointF &grid) const;
  QPoint cellAt(const QPointF &grid) const;

  /**
   * Field image for scalar fields, one pixel per cell.
   */
  QImage fieldImage() const;

  void drawVelocity(QPainter &painter) const;
  void drawGrid(QPainter &painter) const;
  void drawObstacles(QPainter &painter) const;
  void drawParticles(QPainter &painter) const;
  void drawForce(QPainter &painter) const;
  void drawLegend(QPainter &painter) const;

  /**
   * Send the input of a mouse press or drag.
   */
  void handleMouse(QMouseEvent *event);

  void updateHover(const QPointF &position);

 protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;

 public:
  explicit SimulationView(QWidget *parent = nullptr);

  VisualisationMode mode() const { return mode_; }

 public slots:
  /**
   * Show a new frame.
   */
  void setFrame(FramePtr frame);

  /**
   * Show a different field.
   */
  void setMode(VisualisationMode mode);

  /**
   * Update the sidebar toggle without emitting a signal.
   */
  void setSidebarVisible(bool visible);

 signals:
  void modeChanged(VisualisationMode mode);
  void clearParticlesClicked();
  void sidebarToggled(bool visible);

  void obstacleAdded(QPoint cell);
  void obstacleRemoved(QPoint cell);
  void inkAdded(QPoint cell);
  void tracerAdded(QPoint cell);
  void particleAdded(QPointF position);
  void forceApplied(QPoint cell, QPointF direction);

  /**
   * The mouse moved to another cell, a null point if it left the channel.
   */
  void cellHovered(QPoint cell);
};

#endif //REAL_TIME_FLUID_SIMULATION_VIEW_H
