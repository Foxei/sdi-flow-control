#include "MainWindow.h"

#include <QtCore/QSettings>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setUpGui();
  restoreWindowStates();
}

void MainWindow::setUpGui() {
  setMinimumSize(QSize(800, 560));
  resize(1280, 800);

  // Create instances
  splitter_ = new QSplitter(this);
  widget_right_ = new QWidget(splitter_);
  auto *layout_right = new QVBoxLayout(widget_right_);

  simulation_view_ = new SimulationView(splitter_);
  side_bar_ = new SideBar(widget_right_);
  control_bar_ = new ControlBar(widget_right_);
  status_bar_ = new StatusBar(this);
  // end

  // Right side: scrollable sidebar with the restart button below
  auto *scroll_area = new QScrollArea(widget_right_);
  scroll_area->setWidget(side_bar_);
  scroll_area->setWidgetResizable(true);
  scroll_area->setFrameShape(QFrame::NoFrame);
  scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  layout_right->addWidget(scroll_area, 1);
  layout_right->addWidget(control_bar_);
  layout_right->setContentsMargins(0, 0, 0, 0);
  layout_right->setSpacing(0);
  widget_right_->setMinimumWidth(side_bar_->minimumWidth());
  // end

  // Simulation on the left takes the remaining space
  splitter_->addWidget(simulation_view_);
  splitter_->addWidget(widget_right_);
  splitter_->setStretchFactor(0, 1);
  splitter_->setStretchFactor(1, 0);
  splitter_->setSizes({width() - 360, 360});
  splitter_->setChildrenCollapsible(false);
  // end

  // set basic main window stuff
  setStatusBar(status_bar_);
  setCentralWidget(splitter_);
  // end

  connect(simulation_view_, &SimulationView::sidebarToggled, this, &MainWindow::setSidebarVisible);
}

void MainWindow::setSidebarVisible(bool visible) {
  widget_right_->setVisible(visible);
  simulation_view_->setSidebarVisible(visible);
}

void MainWindow::restoreWindowStates() {
  QSettings settings;
  restoreGeometry(settings.value("geometry").toByteArray());
  splitter_->restoreState(settings.value("splitter").toByteArray());
  setSidebarVisible(settings.value("sidebar_visible", true).toBool());
}

void MainWindow::storeWindowState() {
  QSettings settings;
  settings.setValue("geometry", saveGeometry());
  settings.setValue("splitter", splitter_->saveState());
  settings.setValue("sidebar_visible", widget_right_->isVisibleTo(this));
}

void MainWindow::closeEvent(QCloseEvent *event) {
  storeWindowState();
  QMainWindow::closeEvent(event);
}
