#ifndef REAL_TIME_FLUID_SIDE_BAR_H
#define REAL_TIME_FLUID_SIDE_BAR_H

#include "model/Definitions.h"
#include "view/sidebar/CollapsibleElement.h"
#include "view/sidebar/NumberInput.h"
#include "view/sidebar/PopupMessage.h"
#include "view/sidebar/SimulationList.h"

#include <QtWidgets/QFrame>

/**
 * Sidebar with the stored simulations and all simulation parameters. Except
 * for the grid size all values are applied immediately.
 */
class SideBar : public QFrame {
 Q_OBJECT
 private:
  PopupMessage *current_message_ = nullptr; //< Banner for error and info messages
  SimulationList *simulation_list_ = nullptr;

  NumberInput *spin_width_ = nullptr;      //< Grid cells in flow direction
  NumberInput *spin_height_ = nullptr;     //< Grid cells across the channel
  NumberInput *spin_speed_ = nullptr;      //< Inflow speed
  NumberInput *spin_timestep_ = nullptr;   //< Simulated time per second
  NumberInput *spin_viscosity_ = nullptr;  //< Viscosity of the fluid
  NumberInput *spin_diffusion_ = nullptr;  //< Diffusivity of the ink
  NumberInput *spin_iterations_ = nullptr; //< Gauss-Seidel iterations
  NumberInput *spin_interval_ = nullptr;   //< Steps between scale updates
  NumberInput *spin_force_ = nullptr;      //< Force of a mouse drag

  QSize solver_grid_; //< Grid size of the running solver

  /**
   * Setting up gui.
   */
  void setUpGUI();

  /**
   * Connect the inputs to the signals of the sidebar.
   */
  void setUpConnections();

  /**
   * Row of the controls section: key combination and what it does.
   */
  static void addControlRow(QFormLayout &layout, const QString &keys, const QString &action);

  /**
   * Show a hint if the entered grid differs from the running solver.
   */
  void updateGridHint();

 public:
  /**
   * Setup widget gui.
   * @param parent parent widget for deallocation of pointers
   */
  explicit SideBar(QWidget *parent = nullptr);

  SimulationList *simulationList() const { return simulation_list_; }

  /**
   * @return grid size entered by the user
   */
  QSize grid() const;

 public slots:
  /**
   * Show the values of a loaded simulation, without emitting signals.
   */
  void setParameters(SolverParameters parameters);

  /**
   * Shows a message in the banner on top
   */
  void showMessage(const QString &message, MessageTyp type = MessageTyp::Error);

  /**
   * Hide the banner
   */
  void hideMessage();

 signals:
  void flowSpeedChanged(double value);
  void timeStepChanged(double value);
  void viscosityChanged(double value);
  void diffusionChanged(double value);
  void solverIterationsChanged(int value);
  void scalarIntervalChanged(int value);
  void forceChanged(double value);

  /**
   * Enter was pressed in a grid input
   */
  void restartRequested();
};

#endif //REAL_TIME_FLUID_SIDE_BAR_H
