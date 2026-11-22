#include "SideBar.h"

#include "model/Data.h"
#include "view/SizeDefinitions.h"

#include <QtGui/QKeySequence>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

SideBar::SideBar(QWidget *parent) : QFrame(parent) {
  setUpGUI();
  setUpConnections();
}

void SideBar::setUpGUI() {
  // Message banner, hidden until a message is shown
  current_message_ = new PopupMessage(this);
  current_message_->setVisible(false);

  // Stored simulations
  auto *widget_storage = new CollapsibleElement(this);
  widget_storage->setTitle(tr("Simulations"));
  widget_storage->setDescription(tr("Changes to obstacles and parameters are saved"
                                    " automatically in the active simulation."));
  simulation_list_ = new SimulationList(widget_storage);
  widget_storage->getLayout().addRow(simulation_list_);

  // Simulation and boundary conditions
  auto *widget_simulation = new CollapsibleElement(this);
  QFormLayout *layout_simulation = &widget_simulation->getLayout();
  widget_simulation->setTitle(tr("Simulation"));
  widget_simulation->setDescription(tr("Grid size and boundary conditions. A new grid"
                                       " size is applied on restart."));

  spin_width_ = new NumberInput(0, widget_simulation);
  spin_width_->setRange(Data::MIN_GRID_SIZE, Data::MAX_GRID_SIZE);
  spin_width_->setToolTip(tr("Cells in flow direction"));
  spin_height_ = new NumberInput(0, widget_simulation);
  spin_height_->setRange(Data::MIN_GRID_SIZE, Data::MAX_GRID_SIZE);
  spin_height_->setToolTip(tr("Cells across the channel"));
  auto *widget_grid = new QWidget(widget_simulation);
  auto *layout_grid = new QHBoxLayout(widget_grid);
  layout_grid->setContentsMargins(0, 0, 0, 0);
  layout_grid->setSpacing(Size::FORM_SPACING / 2);
  auto *label_times = new QLabel(QStringLiteral("×"), widget_grid);
  label_times->setForegroundRole(QPalette::PlaceholderText);
  layout_grid->addWidget(spin_width_, 1);
  layout_grid->addWidget(label_times);
  layout_grid->addWidget(spin_height_, 1);
  layout_simulation->addRow(tr("Grid"), widget_grid);

  spin_speed_ = new NumberInput(4, widget_simulation);
  spin_speed_->setRange(0, 10);
  spin_speed_->setToolTip(tr("Speed of the fluid entering the channel on the left"));
  layout_simulation->addRow(tr("Flow speed"), spin_speed_);

  spin_timestep_ = new NumberInput(4, widget_simulation);
  spin_timestep_->setRange(0.0001, 10);
  spin_timestep_->setToolTip(tr("Simulated time per second"));
  layout_simulation->addRow(tr("Time step"), spin_timestep_);

  // Fluid
  auto *widget_fluid = new CollapsibleElement(this);
  QFormLayout *layout_fluid = &widget_fluid->getLayout();
  widget_fluid->setTitle(tr("Fluid"));
  widget_fluid->setDescription(tr("Material constants of the fluid and the ink."));

  spin_viscosity_ = new NumberInput(4, widget_fluid);
  spin_viscosity_->setRange(0, 1000);
  spin_viscosity_->setToolTip(tr("Viscosity of the fluid"));
  layout_fluid->addRow(tr("Viscosity"), spin_viscosity_);

  spin_diffusion_ = new NumberInput(4, widget_fluid);
  spin_diffusion_->setRange(0, 100);
  spin_diffusion_->setToolTip(tr("Diffusivity of the ink"));
  layout_fluid->addRow(tr("Diffusivity"), spin_diffusion_);

  // Advanced
  auto *widget_advanced = new CollapsibleElement(this);
  QFormLayout *layout_advanced = &widget_advanced->getLayout();
  widget_advanced->setTitle(tr("Advanced"));
  widget_advanced->setDescription(tr("These settings affect the speed and the"
                                     " stability of the solver."));

  spin_iterations_ = new NumberInput(0, widget_advanced);
  spin_iterations_->setRange(1, 5000);
  spin_iterations_->setToolTip(tr("Iterations of the Gauss-Seidel method per linear system"));
  layout_advanced->addRow(tr("Iterations"), spin_iterations_);

  spin_interval_ = new NumberInput(0, widget_advanced);
  spin_interval_->setRange(1, 1000);
  spin_interval_->setToolTip(tr("Solver steps between updates of the color scales"));
  layout_advanced->addRow(tr("Scale update"), spin_interval_);

  spin_force_ = new NumberInput(2, widget_advanced);
  spin_force_->setRange(0, 1000);
  spin_force_->setToolTip(tr("Force applied when dragging with %1")
                              .arg(QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText)));
  layout_advanced->addRow(tr("Force"), spin_force_);

  // Mouse controls
  auto *widget_controls = new CollapsibleElement(this);
  QFormLayout *layout_controls = &widget_controls->getLayout();
  widget_controls->setTitle(tr("Controls"));
  widget_controls->setDescription(tr("Interact with the fluid using the mouse."));
  QString shift = QKeySequence(Qt::SHIFT).toString(QKeySequence::NativeText);
  QString control = QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText);
  // NativeText of a modifier ends with "+" on Windows and Linux
  shift.remove('+');
  control.remove('+');
  addControlRow(*layout_controls, tr("Left drag"), tr("Release particles"));
  addControlRow(*layout_controls, tr("Right drag"), tr("Release traced particles, add ink in the density view"));
  addControlRow(*layout_controls, tr("%1 + left drag").arg(shift), tr("Draw obstacles"));
  addControlRow(*layout_controls, tr("%1 + right drag").arg(shift), tr("Erase obstacles"));
  addControlRow(*layout_controls, tr("%1 + drag").arg(control), tr("Push the fluid"));

  // Align the fields of all sections by giving all labels the same width,
  // and center each label vertically on its field
  int label_width = 0;
  QList<QWidget *> labels;
  for (CollapsibleElement *section : {widget_simulation, widget_fluid, widget_advanced, widget_controls}) {
    QFormLayout &form = section->getLayout();
    for (int row = 0; row < form.rowCount(); row++) {
      QLayoutItem *label = form.itemAt(row, QFormLayout::LabelRole);
      QLayoutItem *field = form.itemAt(row, QFormLayout::FieldRole);
      if (label && field && label->widget() && field->widget()) {
        labels << label->widget();
        label_width = qMax(label_width, label->widget()->sizeHint().width());
        if (section != widget_controls) {
          label->widget()->setMinimumHeight(field->widget()->minimumSizeHint().height());
        }
      }
    }
  }
  for (QWidget *label : labels) {
    label->setMinimumWidth(label_width);
  }

  // Add Widgets and set layout
  auto *widget_layout = new QVBoxLayout(this);
  widget_layout->addWidget(current_message_);
  widget_layout->addWidget(widget_storage);
  widget_layout->addWidget(widget_simulation);
  widget_layout->addWidget(widget_fluid);
  widget_layout->addWidget(widget_advanced);
  widget_layout->addWidget(widget_controls);
  widget_layout->addStretch(1);
  widget_layout->setContentsMargins(12, 12, 12, 0);

  setMinimumWidth(300);
}

void SideBar::addControlRow(QFormLayout &layout, const QString &keys, const QString &action) {
  auto *label_keys = new QLabel(keys);
  auto *label_action = new QLabel(action);
  label_action->setWordWrap(true);
  label_action->setForegroundRole(QPalette::PlaceholderText);
  layout.addRow(label_keys, label_action);
}

void SideBar::setUpConnections() {
  connect(spin_speed_, &NumberInput::valueChanged, this, &SideBar::flowSpeedChanged);
  connect(spin_timestep_, &NumberInput::valueChanged, this, &SideBar::timeStepChanged);
  connect(spin_viscosity_, &NumberInput::valueChanged, this, &SideBar::viscosityChanged);
  connect(spin_diffusion_, &NumberInput::valueChanged, this, &SideBar::diffusionChanged);
  connect(spin_force_, &NumberInput::valueChanged, this, &SideBar::forceChanged);
  connect(spin_iterations_, &NumberInput::valueChanged, this, [this](double value) {
    emit solverIterationsChanged(static_cast<int>(value));
  });
  connect(spin_interval_, &NumberInput::valueChanged, this, [this](double value) {
    emit scalarIntervalChanged(static_cast<int>(value));
  });
  for (NumberInput *input : {spin_width_, spin_height_}) {
    connect(input, &NumberInput::valueChanged, this, &SideBar::updateGridHint);
    connect(input, &NumberInput::returnPressed, this, &SideBar::restartRequested);
  }
}

QSize SideBar::grid() const {
  return {static_cast<int>(spin_width_->value()), static_cast<int>(spin_height_->value())};
}

void SideBar::updateGridHint() {
  if (grid() != solver_grid_) {
    showMessage(tr("Restart to apply the grid of %1 × %2 cells.")
                    .arg(grid().width()).arg(grid().height()), MessageTyp::OK);
  } else {
    hideMessage();
  }
}

void SideBar::setParameters(SolverParameters parameters) {
  const QList<NumberInput *> inputs = {spin_width_, spin_height_, spin_speed_, spin_timestep_,
                                       spin_viscosity_, spin_diffusion_, spin_iterations_,
                                       spin_interval_, spin_force_};
  for (NumberInput *input : inputs) {
    input->blockSignals(true);
  }
  solver_grid_ = QSize(static_cast<int>(parameters.width), static_cast<int>(parameters.height));
  spin_width_->setValue(static_cast<double>(parameters.width));
  spin_height_->setValue(static_cast<double>(parameters.height));
  spin_speed_->setValue(parameters.flow_speed);
  spin_timestep_->setValue(parameters.target_time_step);
  spin_viscosity_->setValue(parameters.viscosity);
  spin_diffusion_->setValue(parameters.diffusion);
  spin_iterations_->setValue(static_cast<double>(parameters.solver_iterations));
  spin_interval_->setValue(static_cast<double>(parameters.scalar_interval));
  spin_force_->setValue(parameters.force);
  for (NumberInput *input : inputs) {
    input->blockSignals(false);
  }
  updateGridHint();
}

void SideBar::showMessage(const QString &message, MessageTyp type) {
  QString title;
  if (type == MessageTyp::Error) {
    title = tr("Error");
  } else if (type == MessageTyp::OK) {
    title = tr("Information");
  }
  current_message_->setMessage(title, message, type);
  current_message_->show();
}

void SideBar::hideMessage() {
  current_message_->setVisible(false);
}
