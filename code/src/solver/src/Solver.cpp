#include <FluidSolver/Solver.h>

#include <algorithm>
#include <cmath>
#include <limits>

Matrix::Matrix(std::size_t m, std::size_t n)
    : matrix_(Eigen::MatrixXf::Zero(static_cast<Eigen::Index>(m), static_cast<Eigen::Index>(n))) {}

SingleStepMatrix::SingleStepMatrix(std::size_t m, std::size_t n)
    : current_(Eigen::MatrixXf::Zero(static_cast<Eigen::Index>(m), static_cast<Eigen::Index>(n))),
      previous_(Eigen::MatrixXf::Zero(static_cast<Eigen::Index>(m), static_cast<Eigen::Index>(n))) {}

Solver::Solver(std::size_t width, std::size_t height, float flow_speed) :
    width_(width),
    height_(height),
    // Update the maximum values in the first step
    counter_(std::numeric_limits<std::size_t>::max() / 2),
    velocity_x_(width + 2, height + 2),
    velocity_y_(width + 2, height + 2),
    speed_(width + 2, height + 2),
    particle_x_(width + 2, height + 2),
    particle_y_(width + 2, height + 2),
    pressure_(width + 2, height + 2),
    divergence_(width + 2, height + 2),
    blocked_fields_(width + 2, height + 2),
    open_fields_(width + 2, height + 2),
    densities_(width + 2, height + 2) {
  Eigen::MatrixXf &particle_x = *particle_x_.matrix();
  Eigen::MatrixXf &particle_y = *particle_y_.matrix();
  Eigen::MatrixXf &velocity_x = *velocity_x_.current();
  Eigen::MatrixXf &blocked = *blocked_fields_.matrix();

  for (Eigen::Index i = 0; i < particle_x.rows(); i++) {
    for (Eigen::Index j = 0; j < particle_x.cols(); j++) {
      particle_x(i, j) = static_cast<float>(i) + 0.5f;
      particle_y(i, j) = static_cast<float>(j) + 0.5f;
      velocity_x(i, j) = flow_speed;
    }
  }
  // Channel walls
  for (std::size_t i = 1; i < width_ + 1; i++) {
    blocked(i, 0) = 1;
    blocked(i, height_ + 1) = 1;
  }
  *open_fields_.matrix() = 1.0f - blocked.array();
}

float Solver::at(const Eigen::MatrixXf &field, float x, float y) {
  auto i = std::clamp<Eigen::Index>(static_cast<Eigen::Index>(x), 0, field.rows() - 1);
  auto j = std::clamp<Eigen::Index>(static_cast<Eigen::Index>(y), 0, field.cols() - 1);
  return field(i, j);
}

void Solver::updateBoundary(Eigen::MatrixXf *value, FieldDirection direction) {
  const auto w = static_cast<Eigen::Index>(width_);
  const auto h = static_cast<Eigen::Index>(height_);
  Eigen::MatrixXf &v = *value;

  // Nothing flows inside obstacles
  v.block(1, 1, w, h).array() *= open_fields_.matrix()->block(1, 1, w, h).array();

  switch (direction) {
    case FieldDirection::X : {
      v.col(0) = v.col(1);
      v.col(h + 1) = v.col(h);
      v.row(w + 1) = v.row(w);
      v.row(0).setConstant(params_->flow_speed);
      break;
    }
    case FieldDirection::Y : {
      v.col(0) = -v.col(1);
      v.col(h + 1) = -v.col(h);
      v.row(w + 1) = v.row(w);
      v.row(0).setZero();
      break;
    }
    case FieldDirection::NONE : {
      v.col(0) = v.col(1);
      v.col(h + 1) = v.col(h);
      v.row(w + 1) = v.row(w);
      v.row(0) = v.row(1);
      break;
    }
    case FieldDirection::ZERO : {
      v.col(0).setZero();
      v.col(h + 1).setZero();
      v.row(w + 1).setZero();
      v.row(0).setZero();
      break;
    }
  }
}

void Solver::linearSolve(Eigen::MatrixXf *value, const Eigen::MatrixXf *value0,
                         const Eigen::Matrix3f &stencil, FieldDirection direction) {
  Eigen::MatrixXf &v = *value;
  const Eigen::MatrixXf &v0 = *value0;
  const Eigen::MatrixXf &open = *open_fields_.matrix();

  const float c_bottom = stencil(0, 1); // (i, j - 1)
  const float c_left = stencil(1, 0);   // (i - 1, j)
  const float c_right = stencil(1, 2);  // (i + 1, j)
  const float c_top = stencil(2, 1);    // (i, j + 1)
  const float diagonal_neighbour = stencil(1, 1);
  const float diagonal_constant = stencil(0, 0);

  // Without coupling every iteration gives the same result
  bool coupled = c_bottom != 0 || c_left != 0 || c_right != 0 || c_top != 0;
  std::size_t iterations = coupled ? params_->solver_iterations : 1;

  for (std::size_t iteration = 0; iteration < iterations; iteration++) {
    for (std::size_t i = 1; i <= width_; i++) {
      for (std::size_t j = 1; j <= height_; j++) {
        int center = (c_bottom * open(i, j - 1) != 0)
                     + (c_left * open(i - 1, j) != 0)
                     + (c_right * open(i + 1, j) != 0)
                     + (c_top * open(i, j + 1) != 0);
        if (center == 0 && c_bottom != 0) {
          v(i, j) = 0;
          continue;
        }
        float diagonal = static_cast<float>(center) * diagonal_neighbour + diagonal_constant;
        v(i, j) = (v0(i, j)
                   + c_bottom * open(i, j - 1) * v(i, j - 1)
                   + c_left * open(i - 1, j) * v(i - 1, j)
                   + c_right * open(i + 1, j) * v(i + 1, j)
                   + c_top * open(i, j + 1) * v(i, j + 1)) / diagonal;
      }
    }
    updateBoundary(value, direction);
  }
}

void Solver::diffuse(Eigen::MatrixXf *value, const Eigen::MatrixXf *value0,
                     FieldDirection direction, double constant) {
  auto a = static_cast<float>(params_->time_step * constant);
  Eigen::Matrix3f stencil;
  stencil << 1, a, 0,
             a, a, a,
             0, a, 0;
  linearSolve(value, value0, stencil, direction);
}

void Solver::projection() {
  Eigen::MatrixXf &divergence = *divergence_.matrix();
  Eigen::MatrixXf &velocity_x = *velocity_x_.current();
  Eigen::MatrixXf &velocity_y = *velocity_y_.current();
  Eigen::MatrixXf &pressure = *pressure_.matrix();
  const Eigen::MatrixXf &open = *open_fields_.matrix();

  for (std::size_t i = 1; i <= width_; i++) {
    for (std::size_t j = 1; j <= height_; j++) {
      divergence(i, j) = -0.5f * ((velocity_x(i + 1, j) - velocity_x(i - 1, j))
                                  + (velocity_y(i, j + 1) - velocity_y(i, j - 1))) * open(i, j);
      pressure(i, j) = 0;
    }
  }

  updateBoundary(&divergence, FieldDirection::NONE);
  updateBoundary(&pressure, FieldDirection::NONE);

  Eigen::Matrix3f stencil;
  stencil << 0, 1, 0,
             1, 1, 1,
             0, 1, 0;
  linearSolve(&pressure, &divergence, stencil, FieldDirection::NONE);

  for (std::size_t i = 1; i <= width_; i++) {
    for (std::size_t j = 1; j <= height_; j++) {
      velocity_x(i, j) -= 0.5f * ((pressure(i + 1, j) - pressure(i, j)) * open(i + 1, j)
                                  + (pressure(i, j) - pressure(i - 1, j)) * open(i - 1, j));
      velocity_y(i, j) -= 0.5f * ((pressure(i, j + 1) - pressure(i, j)) * open(i, j + 1)
                                  + (pressure(i, j) - pressure(i, j - 1)) * open(i, j - 1));
    }
  }
  updateBoundary(&velocity_x, FieldDirection::X);
  updateBoundary(&velocity_y, FieldDirection::Y);
}

void Solver::convection(Eigen::MatrixXf *value, const Eigen::MatrixXf *value0,
                        FieldDirection direction) {
  const Eigen::MatrixXf &particle_x = *particle_x_.matrix();
  const Eigen::MatrixXf &particle_y = *particle_y_.matrix();
  Eigen::MatrixXf &v = *value;
  const Eigen::MatrixXf &v0 = *value0;
  const auto max_x = static_cast<float>(width_ + 1);
  const auto max_y = static_cast<float>(height_ + 1);

  for (std::size_t i = 1; i < width_ + 1; i++) {
    for (std::size_t j = 1; j < height_ + 1; j++) {
      auto [old_x, old_y] = backprojection(particle_x(i, j), particle_y(i, j));
      old_x = std::clamp(old_x, 1.0f, max_x);
      old_y = std::clamp(old_y, 1.0f, max_y);

      // Bilinear interpolation between the four surrounding cell centers
      auto i0 = static_cast<Eigen::Index>(old_x - 0.5f);
      auto j0 = static_cast<Eigen::Index>(old_y - 0.5f);
      Eigen::Index i1 = i0 + 1;
      Eigen::Index j1 = j0 + 1;

      float right = old_x - particle_x(i0, j0);
      float top = old_y - particle_y(i0, j0);
      float left = 1.0f - right;
      float bottom = 1.0f - top;

      v(i, j) = bottom * (left * v0(i0, j0) + right * v0(i1, j0))
                + top * (left * v0(i0, j1) + right * v0(i1, j1));
    }
  }

  updateBoundary(value, direction);
}

void Solver::step(const SolverParameters &parameters) {
  params_ = &parameters;
  counter_++;

  velocity_x_.swap();
  velocity_y_.swap();
  diffuse(velocity_x_.current(), velocity_x_.previous(), FieldDirection::X, params_->viscosity);
  diffuse(velocity_y_.current(), velocity_y_.previous(), FieldDirection::Y, params_->viscosity);
  projection();

  velocity_x_.swap();
  velocity_y_.swap();
  convection(velocity_x_.current(), velocity_x_.previous(), FieldDirection::X);
  convection(velocity_y_.current(), velocity_y_.previous(), FieldDirection::Y);
  projection();

  densities_.swap();
  convection(densities_.current(), densities_.previous(), FieldDirection::ZERO);
  densities_.swap();
  diffuse(densities_.current(), densities_.previous(), FieldDirection::ZERO, params_->diffusion);

  newPosition();
  newTrajectory();

  if (counter_ >= params_->scalar_interval) {
    counter_ = 0;
    const Eigen::MatrixXf &velocity_x = *velocity_x_.current();
    const Eigen::MatrixXf &velocity_y = *velocity_y_.current();
    max_divergence_ = std::max(0.1f, divergence_.matrix()->cwiseAbs().maxCoeff());
    max_pressure_ = std::max(0.1f, pressure_.matrix()->cwiseAbs().maxCoeff());
    *speed_.matrix() = (velocity_x.array().square() + velocity_y.array().square()).sqrt();
    max_speed_ = speed_.matrix()->maxCoeff();
  }
  params_ = nullptr;
}

bool Solver::advect(const Eigen::Vector2f &from, Eigen::Vector2f &to) {
  const Eigen::MatrixXf &velocity_x = *velocity_x_.current();
  const Eigen::MatrixXf &velocity_y = *velocity_y_.current();
  const Eigen::MatrixXf &blocked = *blocked_fields_.matrix();
  const float time_step = params_->time_step;

  float x = from(0);
  float y = from(1);
  float new_x = x + time_step * at(velocity_x, x, y);
  float new_y = y + time_step * at(velocity_y, x, y);
  to = Eigen::Vector2f(new_x, new_y);

  if (!(new_x < static_cast<float>(width_ + 1) && new_x > 0
        && new_y < static_cast<float>(height_ + 1) && new_y > 0)) {
    return false;
  }
  // Slide along obstacles instead of entering them
  if (at(blocked, new_x, new_y) == 1) {
    if (at(blocked, new_x, y) == 1) {
      new_x = x;
    } else {
      new_y = y;
    }
  } else if (at(blocked, new_x, y) == 1 && at(blocked, x, new_y) == 1) {
    new_x = x;
    new_y = y;
  }
  to = Eigen::Vector2f(new_x, new_y);
  return true;
}

void Solver::newPosition() {
  std::vector<Eigen::Vector2f> moved;
  moved.reserve(new_points_.size());
  for (const Eigen::Vector2f &current : new_points_) {
    Eigen::Vector2f next;
    if (advect(current, next)) {
      moved.push_back(next);
    }
  }
  new_points_.swap(moved);
}

void Solver::newTrajectory() {
  // Traced particles that leave the channel are not moved anymore, only
  // their trace is kept
  std::vector<TrajectoryPoint> moved;
  moved.reserve(trajectory_points_.size());
  for (TrajectoryPoint &current : trajectory_points_) {
    Eigen::Vector2f next;
    if (advect(current.getCurrentPoint(), next)) {
      current.appendPoint(next);
      moved.push_back(std::move(current));
    } else {
      // End the trace where it crosses the channel border
      Eigen::Vector2f from = current.getCurrentPoint();
      float t = 1.0f;
      for (int axis = 0; axis < 2; axis++) {
        float low = 1.0f;
        float high = static_cast<float>((axis == 0 ? width_ : height_) + 1);
        float delta = next(axis) - from(axis);
        if (next(axis) < low && delta != 0) {
          t = std::min(t, (low - from(axis)) / delta);
        } else if (next(axis) > high && delta != 0) {
          t = std::min(t, (high - from(axis)) / delta);
        }
      }
      current.appendPoint(from + std::clamp(t, 0.0f, 1.0f) * (next - from));
      finished_traces_.push_back(current.getList());
    }
  }
  trajectory_points_.swap(moved);
}

std::pair<float, float> Solver::backprojection(float x, float y) {
  const Eigen::MatrixXf &blocked = *blocked_fields_.matrix();
  const Eigen::MatrixXf &velocity_x = *velocity_x_.current();
  const Eigen::MatrixXf &velocity_y = *velocity_y_.current();
  const auto max_x = static_cast<float>(width_ + 1);
  const auto max_y = static_cast<float>(height_ + 1);

  const int checkpoints = 20;
  const float time_step = params_->time_step;
  const float short_time_step = time_step / (checkpoints + 1);

  float x_i = x;
  float y_i = y;
  float x_velocity = x;
  float y_velocity = y;

  for (int k = 0; k <= checkpoints; k++) {
    float d_x = short_time_step * at(velocity_x, x_velocity, y_velocity);
    float d_y = short_time_step * at(velocity_y, x_velocity, y_velocity);
    if (at(blocked, x_i - d_x, y_i - d_y) == 1) {
      x_velocity = x_i;
      y_velocity = y_i;
      d_x = time_step * at(velocity_x, x_velocity, y_velocity);
      d_y = time_step * at(velocity_y, x_velocity, y_velocity);
    }
    x_i = std::clamp(x_i - d_x, 1.0f, max_x);
    y_i = std::clamp(y_i - d_y, 1.0f, max_y);
  }
  return {x_i, y_i};
}

bool Solver::setBlocked(int i, int j, bool blocked) {
  if (i < 1 || j < 1 || i > static_cast<int>(width_) || j > static_cast<int>(height_)) {
    return false;
  }
  (*blocked_fields_.matrix())(i, j) = blocked ? 1.0f : 0.0f;
  (*open_fields_.matrix())(i, j) = blocked ? 0.0f : 1.0f;
  return true;
}

bool Solver::addInk(int i, int j) {
  if (i < 1 || j < 1 || i > static_cast<int>(width_) || j > static_cast<int>(height_)) {
    return false;
  }
  (*densities_.current())(i, j) = 1.0f;
  return true;
}

bool Solver::addForce(int i, int j, const Eigen::Vector2f &force) {
  if (i < 1 || j < 1 || i > static_cast<int>(width_) || j > static_cast<int>(height_)) {
    return false;
  }
  // The previous step only holds the force, it is added to the current step
  Eigen::MatrixXf &velocity_x0 = *velocity_x_.previous();
  Eigen::MatrixXf &velocity_y0 = *velocity_y_.previous();
  velocity_x0.setZero();
  velocity_y0.setZero();
  velocity_x0(i, j) = force(0);
  velocity_y0(i, j) = force(1);
  *velocity_x_.current() += velocity_x0;
  *velocity_y_.current() += velocity_y0;
  return true;
}

void Solver::clearParticles() {
  new_points_.clear();
  trajectory_points_.clear();
  finished_traces_.clear();
}
