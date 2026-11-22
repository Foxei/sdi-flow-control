#ifndef REAL_TIME_FLUID_MAIN_WINDOW_H
#define REAL_TIME_FLUID_MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>

#include "view/control/Controlbar.h"
#include "view/sidebar/SideBar.h"
#include "view/simulation/SimulationView.h"
#include "view/status/StatusBar.h"

/**
 * Contains the simulation on the left and the sidebar on the right.
 */
class MainWindow : public QMainWindow {
 Q_OBJECT
 private:
  SimulationView *simulation_view_ = nullptr; //< Simulation display and mouse input
  SideBar *side_bar_ = nullptr;               //< All parameters
  ControlBar *control_bar_ = nullptr;         //< Restart button below the sidebar
  StatusBar *status_bar_ = nullptr;           //< Messages and cell values
  QSplitter *splitter_ = nullptr;
  QWidget *widget_right_ = nullptr;           //< Sidebar and control bar

  /**
   * Will connect all components to the finals windows
   */
  void setUpGui();

  /**
   * @brief Will restore the window state from the last session.
   */
  void restoreWindowStates();

  /**
   * @brief Will store the window state of the current session.
   */
  void storeWindowState();

 protected:
  /**
   * @brief Store the window state so it can be restored at the next start.
   */
  void closeEvent(QCloseEvent *event) override;

 public:
  /**
   * Will setup gui and use super constructor
   * @param parent Parent QWidget
   */
  explicit MainWindow(QWidget *parent = nullptr);

  SimulationView *getSimulationView() const { return simulation_view_; }
  SideBar *getSidebar() const { return side_bar_; }
  ControlBar *getControlBar() const { return control_bar_; }
  StatusBar *getStatusbar() const { return status_bar_; }

 public slots:
  /**
   * Show or hide the sidebar, the simulation takes the free space.
   */
  void setSidebarVisible(bool visible);
};

#endif // REAL_TIME_FLUID_MAIN_WINDOW_H
