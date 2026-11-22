#ifndef REAL_TIME_FLUID_WORKER_H
#define REAL_TIME_FLUID_WORKER_H

#include "model/Data.h"
#include "model/Definitions.h"

#include <FluidSolver/Solver.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QStringList>

#include <memory>

/**
 * Owns the solver and the stored simulations and runs in its own thread.
 * All slots are called through queued connections, so they never overlap
 * with a solver step. After each step a Frame snapshot is sent to the GUI.
 */
class Worker : public QObject {
 Q_OBJECT
 private:
  Data data_;                       //< Stored simulations
  std::unique_ptr<Solver> solver_;  //< Solver of the active simulation
  QElapsedTimer step_timer_;        //< Real time between two steps
  double step_rate_ = 0;            //< Smoothed steps per second

  /**
   * Create a new solver for the active simulation and add its obstacles.
   * Obstacles outside the grid are dropped.
   */
  void createSolver();

  /**
   * Send the active simulation, the stored names and a new frame to the GUI.
   */
  void publishSimulation();

  /**
   * Snapshot of the current solver state.
   */
  FramePtr createFrame() const;

 public:
  /**
   * Load the stored simulations of the user, or the presets on first start.
   */
  Worker();

  /**
   * File with the simulations of the user.
   */
  static QString storagePath();

  /**
   * Store all simulations. Only call while the worker thread is stopped.
   * @return true if successful
   */
  bool save() const;

 public slots:
  /**
   * Send the initial state to the GUI.
   */
  void start();

  /**
   * Calculate one solver step and emit frameReady.
   */
  void step();

  void addObstacle(QPoint cell);
  void removeObstacle(QPoint cell);
  void clearObstacles();
  void addInk(QPoint cell);
  void addTracer(QPoint cell);
  void addParticle(QPointF position);

  /**
   * Push the fluid in a cell.
   * @param cell grid cell
   * @param direction unit vector of the push, scaled with the force parameter
   */
  void addForce(QPoint cell, QPointF direction);

  void clearParticles();

  /**
   * Restart the solver with a new grid size, obstacles inside the grid stay.
   */
  void restart(QSize grid);

  void loadSimulation(const QString &name);
  void duplicateSimulation();
  void removeSimulation(const QString &name);
  void renameSimulation(const QString &old_name, const QString &new_name);

  void setFlowSpeed(double value);
  void setTimeStep(double value);
  void setViscosity(double value);
  void setDiffusion(double value);
  void setSolverIterations(int value);
  void setScalarInterval(int value);
  void setForce(double value);

 signals:
  /**
   * A new snapshot is ready to be drawn.
   */
  void frameReady(FramePtr frame);

  /**
   * A step requested with step() is finished, after its frameReady.
   */
  void stepped();

  /**
   * A simulation was loaded or restarted.
   * @param parameters its parameters
   */
  void simulationLoaded(SolverParameters parameters);

  /**
   * The list of stored simulations or the active one changed.
   */
  void simulationsChanged(const QStringList &names, const QString &current);

  /**
   * A request could not be done.
   * @param message description for the user
   */
  void error(const QString &message);
};

#endif //REAL_TIME_FLUID_WORKER_H
