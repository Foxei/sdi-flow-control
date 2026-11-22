#include "MainController.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <algorithm>

MainController::MainController(MainWindow &window) : window_(window) {
  worker_ = new Worker();
  worker_->moveToThread(&thread_);

  setConnections();

  thread_.start();
  QMetaObject::invokeMethod(worker_, &Worker::start, Qt::QueuedConnection);
  requestStep();
}

MainController::~MainController() {
  thread_.quit();
  thread_.wait();
  // The thread is stopped, the worker can be used from here
  if (!worker_->save()) {
    qWarning() << "Could not store the simulations to" << Worker::storagePath();
  }
  delete worker_;
}

void MainController::setConnections() {
  SimulationView *view = window_.getSimulationView();
  SideBar *sidebar = window_.getSidebar();
  SimulationList *list = sidebar->simulationList();
  ControlBar *control_bar = window_.getControlBar();
  StatusBar *status_bar = window_.getStatusbar();

  // The step -> draw -> step loop
  connect(this, &MainController::stepRequested, worker_, &Worker::step);
  connect(worker_, &Worker::frameReady, this, &MainController::frameReady);
  connect(worker_, &Worker::stepped, this, &MainController::stepFinished);

  // Mouse input on the simulation
  connect(view, &SimulationView::obstacleAdded, worker_, &Worker::addObstacle);
  connect(view, &SimulationView::obstacleRemoved, worker_, &Worker::removeObstacle);
  connect(view, &SimulationView::inkAdded, worker_, &Worker::addInk);
  connect(view, &SimulationView::tracerAdded, worker_, &Worker::addTracer);
  connect(view, &SimulationView::particleAdded, worker_, &Worker::addParticle);
  connect(view, &SimulationView::forceApplied, worker_, &Worker::addForce);
  connect(view, &SimulationView::clearParticlesClicked, worker_, &Worker::clearParticles);
  connect(view, &SimulationView::cellHovered, this, [this](QPoint cell) {
    hover_cell_ = cell;
    updateCellInformation();
  });

  // Parameters
  connect(sidebar, &SideBar::flowSpeedChanged, worker_, &Worker::setFlowSpeed);
  connect(sidebar, &SideBar::timeStepChanged, worker_, &Worker::setTimeStep);
  connect(sidebar, &SideBar::viscosityChanged, worker_, &Worker::setViscosity);
  connect(sidebar, &SideBar::diffusionChanged, worker_, &Worker::setDiffusion);
  connect(sidebar, &SideBar::solverIterationsChanged, worker_, &Worker::setSolverIterations);
  connect(sidebar, &SideBar::scalarIntervalChanged, worker_, &Worker::setScalarInterval);
  connect(sidebar, &SideBar::forceChanged, worker_, &Worker::setForce);
  connect(worker_, &Worker::simulationLoaded, sidebar, &SideBar::setParameters);

  // Restart and obstacles
  auto restart = [this, sidebar, status_bar] {
    QMetaObject::invokeMethod(worker_, "restart", Qt::QueuedConnection, Q_ARG(QSize, sidebar->grid()));
    status_bar->displayMessage(tr("Simulation restarted"), MessageTyp::OK, 3000);
  };
  connect(control_bar, &ControlBar::restartClicked, this, restart);
  connect(sidebar, &SideBar::restartRequested, this, restart);
  connect(control_bar, &ControlBar::clearObstaclesClicked, worker_, &Worker::clearObstacles);

  // Stored simulations
  connect(list, &SimulationList::simulationSelected, worker_, &Worker::loadSimulation);
  connect(list, &SimulationList::duplicateClicked, worker_, &Worker::duplicateSimulation);
  connect(list, &SimulationList::deleteClicked, worker_, &Worker::removeSimulation);
  connect(list, &SimulationList::simulationRenamed, worker_, &Worker::renameSimulation);
  connect(worker_, &Worker::simulationsChanged, list, &SimulationList::setSimulations);
  connect(worker_, &Worker::error, this, [sidebar, status_bar](const QString &message) {
    sidebar->showMessage(message, MessageTyp::Error);
    status_bar->displayMessage(message, MessageTyp::Error, 5000);
  });
}

void MainController::frameReady(FramePtr frame) {
  frame_ = frame;
  window_.getSimulationView()->setFrame(frame);
  window_.getStatusbar()->displayRate(frame->step_rate);
  updateCellInformation();
}

void MainController::stepFinished() {
  step_pending_ = false;
  scheduleStep();
}

void MainController::scheduleStep() {
  qint64 interval = 1000 / MAX_STEP_RATE;
  qint64 elapsed = step_timer_.isValid() ? step_timer_.elapsed() : interval;
  QTimer::singleShot(std::max<qint64>(0, interval - elapsed), Qt::PreciseTimer, this, &MainController::requestStep);
}

void MainController::requestStep() {
  if (step_pending_) {
    return;
  }
  step_pending_ = true;
  step_timer_.start();
  emit stepRequested();
}

void MainController::updateCellInformation() {
  StatusBar *status_bar = window_.getStatusbar();
  if (!frame_ || hover_cell_.isNull() || hover_cell_.x() > frame_->width || hover_cell_.y() > frame_->height) {
    status_bar->displayCell(QPoint(), 0, 0);
    return;
  }
  status_bar->displayCell(hover_cell_,
                          frame_->speed(hover_cell_.x(), hover_cell_.y()),
                          frame_->pressure(hover_cell_.x(), hover_cell_.y()));
}
