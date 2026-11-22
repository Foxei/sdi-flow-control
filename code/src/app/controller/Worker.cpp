#include "Worker.h"

#include <QtCore/QStandardPaths>

#include <algorithm>
#include <cmath>

Worker::Worker() {
  Data presets;
  presets.loadFromFile(QStringLiteral(":presets/simulations.json"));
  if (!data_.loadFromFile(storagePath())) {
    data_ = presets;
  } else if (data_.presetVersion() < presets.presetVersion()) {
    // Presets added in a newer version of the app
    QString current = data_.currentName();
    data_.addPresets(presets);
    data_.select(current);
  }
  createSolver();
}

QString Worker::storagePath() {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/simulations.json";
}

bool Worker::save() const {
  return data_.storeToFile(storagePath());
}

void Worker::createSolver() {
  Simulation &simulation = data_.current();
  const SolverParameters &p = simulation.parameters;
  solver_ = std::make_unique<Solver>(p.width, p.height, p.flow_speed);
  simulation.obstacles.removeIf([this](const QPoint &cell) {
    return !solver_->setBlocked(cell.x(), cell.y(), true);
  });
  step_timer_.invalidate();
}

void Worker::publishSimulation() {
  emit simulationLoaded(data_.current().parameters);
  emit simulationsChanged(data_.names(), data_.currentName());
  emit frameReady(createFrame());
}

FramePtr Worker::createFrame() const {
  auto frame = std::make_shared<Frame>();
  frame->width = static_cast<int>(solver_->width());
  frame->height = static_cast<int>(solver_->height());
  frame->velocity_x = solver_->velocityX();
  frame->velocity_y = solver_->velocityY();
  frame->speed = solver_->speed();
  frame->pressure = solver_->pressure();
  frame->divergence = solver_->divergence();
  frame->density = solver_->densities();
  frame->blocked = solver_->blockedFields();
  frame->particles = solver_->newPoints();
  frame->trajectories.reserve(solver_->trajectoryPoints().size());
  for (const TrajectoryPoint &trajectory : solver_->trajectoryPoints()) {
    frame->trajectories.push_back(trajectory.getList());
  }
  frame->finished_traces = solver_->finishedTraces();
  frame->max_speed = solver_->maxSpeed();
  frame->max_pressure = solver_->maxPressure();
  frame->max_divergence = solver_->maxDivergence();
  frame->step_rate = step_rate_;
  return frame;
}

void Worker::start() {
  publishSimulation();
}

void Worker::step() {
  // Advance the simulation by the real time since the last step, so the
  // speed of the simulation does not depend on the step rate
  double elapsed = step_timer_.isValid() ? step_timer_.nsecsElapsed() / 1e9 : 1.0 / 60.0;
  step_timer_.start();
  elapsed = std::clamp(elapsed, 1.0 / 240.0, 0.1);
  double rate = 1.0 / elapsed;
  step_rate_ = step_rate_ <= 0 ? rate : 0.9 * step_rate_ + 0.1 * rate;

  SolverParameters &p = data_.current().parameters;
  p.time_step = static_cast<float>(p.target_time_step * elapsed);
  solver_->step(p);
  emit frameReady(createFrame());
  emit stepped();
}

void Worker::addObstacle(QPoint cell) {
  QList<QPoint> &obstacles = data_.current().obstacles;
  if (!obstacles.contains(cell) && solver_->setBlocked(cell.x(), cell.y(), true)) {
    obstacles.append(cell);
  }
}

void Worker::removeObstacle(QPoint cell) {
  if (data_.current().obstacles.removeAll(cell) > 0) {
    solver_->setBlocked(cell.x(), cell.y(), false);
  }
}

void Worker::clearObstacles() {
  QList<QPoint> &obstacles = data_.current().obstacles;
  for (const QPoint &cell : obstacles) {
    solver_->setBlocked(cell.x(), cell.y(), false);
  }
  obstacles.clear();
}

void Worker::addInk(QPoint cell) {
  solver_->addInk(cell.x(), cell.y());
}

void Worker::addTracer(QPoint cell) {
  solver_->addNewTrajectoryPoint(Eigen::Vector2f(cell.x() + 0.5f, cell.y() + 0.5f));
}

void Worker::addParticle(QPointF position) {
  solver_->addNewPoint(Eigen::Vector2f(position.x(), position.y()));
}

void Worker::addForce(QPoint cell, QPointF direction) {
  float force = data_.current().parameters.force;
  solver_->addForce(cell.x(), cell.y(),
                    Eigen::Vector2f(direction.x(), direction.y()) * force);
}

void Worker::clearParticles() {
  solver_->clearParticles();
}

void Worker::restart(QSize grid) {
  SolverParameters &p = data_.current().parameters;
  p.width = static_cast<std::size_t>(std::clamp(grid.width(), Data::MIN_GRID_SIZE, Data::MAX_GRID_SIZE));
  p.height = static_cast<std::size_t>(std::clamp(grid.height(), Data::MIN_GRID_SIZE, Data::MAX_GRID_SIZE));
  createSolver();
  publishSimulation();
}

void Worker::loadSimulation(const QString &name) {
  if (name == data_.currentName() || !data_.select(name)) {
    return;
  }
  createSolver();
  publishSimulation();
}

void Worker::duplicateSimulation() {
  // The copy becomes active, the solver keeps running
  data_.duplicateCurrent();
  emit simulationsChanged(data_.names(), data_.currentName());
}

void Worker::removeSimulation(const QString &name) {
  if (!data_.contains(name)) {
    return;
  }
  if (data_.names().size() <= 1) {
    emit error(tr("The last simulation can not be deleted."));
    return;
  }
  bool was_current = name == data_.currentName();
  data_.remove(name);
  if (was_current) {
    createSolver();
    publishSimulation();
  } else {
    emit simulationsChanged(data_.names(), data_.currentName());
  }
}

void Worker::renameSimulation(const QString &old_name, const QString &new_name) {
  QString name = new_name.trimmed();
  if (name == old_name) {
    return;
  }
  if (name.isEmpty()) {
    emit error(tr("A simulation needs a name."));
  } else if (data_.contains(name)) {
    emit error(tr("A simulation named \"%1\" already exists.").arg(name));
  } else {
    data_.rename(old_name, name);
  }
  // Also resets the list if the rename was rejected
  emit simulationsChanged(data_.names(), data_.currentName());
}

void Worker::setFlowSpeed(double value) {
  data_.current().parameters.flow_speed = static_cast<float>(value);
}

void Worker::setTimeStep(double value) {
  data_.current().parameters.target_time_step = static_cast<float>(value);
}

void Worker::setViscosity(double value) {
  data_.current().parameters.viscosity = static_cast<float>(value);
}

void Worker::setDiffusion(double value) {
  data_.current().parameters.diffusion = static_cast<float>(value);
}

void Worker::setSolverIterations(int value) {
  data_.current().parameters.solver_iterations = static_cast<std::size_t>(std::max(1, value));
}

void Worker::setScalarInterval(int value) {
  data_.current().parameters.scalar_interval = static_cast<std::size_t>(std::max(1, value));
}

void Worker::setForce(double value) {
  data_.current().parameters.force = static_cast<float>(value);
}
