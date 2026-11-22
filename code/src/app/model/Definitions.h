#ifndef REAL_TIME_FLUID_DEFINITIONS_H
#define REAL_TIME_FLUID_DEFINITIONS_H

#include <FluidSolver/SolverParameters.h>

#include <Eigen/Dense>

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QPoint>

#include <memory>
#include <vector>

/**
 * A stored simulation: parameters and obstacles
 */
struct Simulation {
  SolverParameters parameters;
  QList<QPoint> obstacles; //< Blocked cells, 1 <= x <= width, 1 <= y <= height
};

/**
 * All fields that can be displayed
 */
enum class VisualisationMode {
  Divergence, //< Divergence of the velocity field
  Pressure,   //< Pressure field
  Velocity,   //< Velocity field as arrows
  Densities   //< Ink density
};

/**
 * Immutable snapshot of the solver state after a step. The worker thread
 * creates it, the GUI thread only reads it, so drawing never races with the
 * solver.
 */
struct Frame {
  int width = 0;  //< Cells in x direction
  int height = 0; //< Cells in y direction
  Eigen::MatrixXf velocity_x;
  Eigen::MatrixXf velocity_y;
  Eigen::MatrixXf speed;
  Eigen::MatrixXf pressure;
  Eigen::MatrixXf divergence;
  Eigen::MatrixXf density;
  Eigen::MatrixXf blocked;
  std::vector<Eigen::Vector2f> particles;
  std::vector<std::vector<Eigen::Vector2f>> trajectories;    //< Traces of moving particles
  std::vector<std::vector<Eigen::Vector2f>> finished_traces; //< Traces of particles that left the channel
  float max_speed = 1;
  float max_pressure = 1;
  float max_divergence = 1;
  double step_rate = 0; //< Solver steps per second
};

using FramePtr = std::shared_ptr<const Frame>;

Q_DECLARE_METATYPE(SolverParameters)
Q_DECLARE_METATYPE(FramePtr)

#endif //REAL_TIME_FLUID_DEFINITIONS_H
