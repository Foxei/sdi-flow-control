#ifndef REAL_TIME_FLUID_MAIN_CONTROLLER_H
#define REAL_TIME_FLUID_MAIN_CONTROLLER_H

#include "controller/Worker.h"
#include "view/MainWindow.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QThread>

/**
 * Connects the interface with the worker thread and drives the
 * step -> draw -> step loop.
 */
class MainController : public QObject {
 Q_OBJECT
 private:
  MainWindow &window_;
  QThread thread_;            //< Thread the worker lives in
  Worker *worker_ = nullptr;  //< Solver and stored simulations
  FramePtr frame_;            //< Last frame, for the values in the status bar
  QPoint hover_cell_;         //< Cell under the mouse
  bool step_pending_ = false; //< A step is requested and not finished yet
  QElapsedTimer step_timer_;  //< Time since the last step was requested

  /**
   * Maximum steps per second, the simulation time does not depend on it
   */
  static constexpr int MAX_STEP_RATE = 60;

  void setConnections();

  /**
   * Request the next step, not faster than MAX_STEP_RATE.
   */
  void scheduleStep();

  /**
   * Show the values of the hovered cell in the status bar.
   */
  void updateCellInformation();

 private slots:
  void frameReady(FramePtr frame);
  void stepFinished();
  void requestStep();

 public:
  /**
   * Create the worker thread, connect the interface and start the simulation.
   * @param window MainWindow with all components
   */
  explicit MainController(MainWindow &window);

  /**
   * Stop the worker thread and store the simulations.
   */
  ~MainController() override;

 signals:
  void stepRequested();
};

#endif //REAL_TIME_FLUID_MAIN_CONTROLLER_H
