#ifndef REAL_TIME_FLUID_CONTROL_BAR_H
#define REAL_TIME_FLUID_CONTROL_BAR_H

#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

/**
 * Bar below the sidebar with the primary restart button
 */
class ControlBar : public QWidget {
 Q_OBJECT
 private:
  QPushButton *button_restart_ = nullptr;         //< Restart with the entered grid
  QPushButton *button_clear_obstacles_ = nullptr; //< Remove all obstacles

  /**
   * Set up gui.
   */
  void setUpGui();

 public:
  /**
   * Setup widget gui.
   * @param parent parent widget for deallocation of pointers
   */
  explicit ControlBar(QWidget *parent = nullptr);

 signals:
  void restartClicked();
  void clearObstaclesClicked();
};

#endif //REAL_TIME_FLUID_CONTROL_BAR_H
