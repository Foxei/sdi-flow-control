#ifndef FLUID_SOLVER_SOLVER_H
#define FLUID_SOLVER_SOLVER_H

#include <FluidSolver/Matrix.h>
#include <FluidSolver/SolverParameters.h>
#include <FluidSolver/TrajectoryPoint.h>

#include <Eigen/Dense>

#include <cstddef>
#include <utility>
#include <vector>

/**
 * Solves the incompressible Navier-Stokes equations in a 2D channel.
 * The equation is split into diffusion, projection and convection steps, see
 * "Stable Fluids" by Jos Stam for the idea.
 *
 * All fields have one layer of ghost cells, so cell (i, j) with
 * 1 <= i <= width and 1 <= j <= height is inside the channel. Row j = 0 and
 * j = height + 1 are the channel walls.
 */
class Solver {
 private:
  std::size_t width_;   //< Cells in x direction (without ghost cells)
  std::size_t height_;  //< Cells in y direction (without ghost cells)
  const SolverParameters *params_ = nullptr; //< Parameters of the running step

  std::size_t counter_; //< Steps since the maximum values were last updated
  float max_speed_ = 1;      //< Maximum speed in the channel
  float max_divergence_ = 1; //< Maximum absolute divergence
  float max_pressure_ = 1;   //< Maximum absolute pressure

  SingleStepMatrix velocity_x_; //< Velocity in x direction
  SingleStepMatrix velocity_y_; //< Velocity in y direction
  Matrix speed_;                //< Absolute speed of each cell
  Matrix particle_x_;           //< x coordinate of each cell center
  Matrix particle_y_;           //< y coordinate of each cell center
  Matrix pressure_;             //< Pressure of each cell
  Matrix divergence_;           //< Divergence of each cell
  Matrix blocked_fields_;       //< 1 for cells blocked by an obstacle or wall, else 0
  Matrix open_fields_;          //< 1 - blocked_fields_, cached for the linear solver
  SingleStepMatrix densities_;  //< Ink density of each cell

  std::vector<Eigen::Vector2f> new_points_;        //< Free particles
  std::vector<TrajectoryPoint> trajectory_points_; //< Particles that leave a trace
  std::vector<std::vector<Eigen::Vector2f>> finished_traces_; //< Traces of particles that left the channel

  /**
   * Solves the projection step: removes the divergence of the velocity field.
   */
  void projection();

  /**
   * Solves a linear system of equations with the Gauss-Seidel method. The
   * stencil only couples cells that are not blocked.
   * @param value unknown field, also the start value
   * @param value0 right hand side
   * @param stencil 3x3 difference star. (0,0) is added to the diagonal,
   *        (1,1) is multiplied with the number of open neighbours, the
   *        direct neighbours are the coupling coefficients.
   * @param direction boundary handling after each iteration
   */
  void linearSolve(Eigen::MatrixXf *value, const Eigen::MatrixXf *value0,
                   const Eigen::Matrix3f &stencil, FieldDirection direction);

  /**
   * Updates the ghost cells and zeroes all blocked cells.
   * @param value field to update
   * @param direction boundary handling
   */
  void updateBoundary(Eigen::MatrixXf *value, FieldDirection direction);

  /**
   * Solves the diffusion step.
   * @param value new field
   * @param value0 field of the last step
   * @param direction boundary handling
   * @param constant diffusion constant (viscosity or diffusivity)
   */
  void diffuse(Eigen::MatrixXf *value, const Eigen::MatrixXf *value0,
               FieldDirection direction, double constant);

  /**
   * Solves the convection step (semi-Lagrangian).
   * @param value new field
   * @param value0 field of the last step
   * @param direction boundary handling
   */
  void convection(Eigen::MatrixXf *value, const Eigen::MatrixXf *value0,
                  FieldDirection direction);

  /**
   * Moves all free particles along the velocity field, particles that leave
   * the channel are removed.
   */
  void newPosition();

  /**
   * Moves all traced particles along the velocity field. Particles that leave
   * the channel are removed, their trace is kept in finished_traces_.
   */
  void newTrajectory();

  /**
   * Moves a particle one time step along the velocity field. Obstacles stop
   * the movement in the blocked direction.
   * @param from current position
   * @param to new position
   * @return true if the new position is inside the channel
   */
  bool advect(const Eigen::Vector2f &from, Eigen::Vector2f &to);

  /**
   * Traces a point back in time along the velocity field (Euler steps).
   * Adapts the step if there is an obstacle in the way.
   * @param x x start coordinate
   * @param y y start coordinate
   * @return back traced position
   */
  std::pair<float, float> backprojection(float x, float y);

  /**
   * Field value at a position, clamped to the field
   */
  static float at(const Eigen::MatrixXf &field, float x, float y);

 public:
  /**
   * Creates a solver with a channel full of fluid flowing with flow_speed.
   * @param width cells in x direction
   * @param height cells in y direction
   * @param flow_speed initial speed of the fluid
   */
  Solver(std::size_t width, std::size_t height, float flow_speed);

  /**
   * Calculates one time step.
   * @param parameters parameters of this step
   */
  void step(const SolverParameters &parameters);

  /**
   * Blocks or unblocks a cell.
   * @param i cell index in x direction, 1 <= i <= width
   * @param j cell index in y direction, 1 <= j <= height
   * @param blocked true for an obstacle
   * @return false if the cell is outside the channel
   */
  bool setBlocked(int i, int j, bool blocked);

  /**
   * Adds ink to a cell.
   * @return false if the cell is outside the channel
   */
  bool addInk(int i, int j);

  /**
   * Adds an external force to a cell.
   * @param force force vector, already scaled
   * @return false if the cell is outside the channel
   */
  bool addForce(int i, int j, const Eigen::Vector2f &force);

  /**
   * Adds a free particle.
   * @param point position in grid coordinates
   */
  void addNewPoint(const Eigen::Vector2f &point) { new_points_.push_back(point); }

  /**
   * Adds a particle that leaves a trace.
   * @param point start position in grid coordinates
   */
  void addNewTrajectoryPoint(const Eigen::Vector2f &point) { trajectory_points_.emplace_back(point); }

  /**
   * Removes all particles and traces.
   */
  void clearParticles();

  std::size_t width() const { return width_; }
  std::size_t height() const { return height_; }
  float maxSpeed() const { return max_speed_; }
  float maxDivergence() const { return max_divergence_; }
  float maxPressure() const { return max_pressure_; }

  const Eigen::MatrixXf &velocityX() { return *velocity_x_.current(); }
  const Eigen::MatrixXf &velocityY() { return *velocity_y_.current(); }
  const Eigen::MatrixXf &speed() { return *speed_.matrix(); }
  const Eigen::MatrixXf &pressure() { return *pressure_.matrix(); }
  const Eigen::MatrixXf &divergence() { return *divergence_.matrix(); }
  const Eigen::MatrixXf &densities() { return *densities_.current(); }
  const Eigen::MatrixXf &blockedFields() { return *blocked_fields_.matrix(); }
  const std::vector<Eigen::Vector2f> &newPoints() const { return new_points_; }
  const std::vector<TrajectoryPoint> &trajectoryPoints() const { return trajectory_points_; }
  const std::vector<std::vector<Eigen::Vector2f>> &finishedTraces() const { return finished_traces_; }
};

#endif //FLUID_SOLVER_SOLVER_H
