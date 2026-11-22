#include "SimulationList.h"

#include "view/SizeDefinitions.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

SimulationList::SimulationList(QWidget *parent) : QWidget(parent) {
  setUpGui();
}

void SimulationList::setUpGui() {
  list_ = new QListWidget(this);
  list_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setUniformItemSizes(true);
  list_->setSpacing(1);
  list_->setToolTip(tr("Click to load a simulation, double-click to rename it."));
  int row_height = list_->fontMetrics().height() + 10;
  list_->setFixedHeight(row_height * 5 + 2 * list_->frameWidth() + 6);

  button_duplicate_ = new QPushButton(QIcon(":icons/add.svg"), tr("Duplicate"), this);
  button_duplicate_->setToolTip(tr("Store a copy of the active simulation and continue with the copy."));
  button_delete_ = new QPushButton(QIcon(":icons/remove.svg"), tr("Delete"), this);
  button_delete_->setToolTip(tr("Delete the selected simulation."));
  for (QPushButton *button : {button_duplicate_, button_delete_}) {
    button->setMinimumHeight(Size::INPUT_HEIGHT);
    button->setFocusPolicy(Qt::TabFocus);
  }

  auto *layout_buttons = new QHBoxLayout();
  layout_buttons->setSpacing(Size::FORM_SPACING / 2);
  layout_buttons->addWidget(button_duplicate_, 1);
  layout_buttons->addWidget(button_delete_, 1);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(Size::FORM_SPACING / 2);
  layout->addWidget(list_);
  layout->addLayout(layout_buttons);

  connect(list_, &QListWidget::currentRowChanged, this, [this](int row) {
    if (!updating_ && row >= 0 && row < names_.size()) {
      emit simulationSelected(names_.at(row));
    }
  });
  connect(list_, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
    int row = list_->row(item);
    if (!updating_ && row >= 0 && row < names_.size()) {
      emit simulationRenamed(names_.at(row), item->text());
    }
  });
  connect(button_duplicate_, &QPushButton::clicked, this, &SimulationList::duplicateClicked);
  connect(button_delete_, &QPushButton::clicked, this, [this] {
    int row = list_->currentRow();
    if (row >= 0 && row < names_.size()) {
      emit deleteClicked(names_.at(row));
    }
  });
}

void SimulationList::setSimulations(const QStringList &names, const QString &current) {
  updating_ = true;
  names_ = names;
  list_->clear();
  for (const QString &name : names) {
    auto *item = new QListWidgetItem(name, list_);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
  }
  list_->setCurrentRow(static_cast<int>(names.indexOf(current)));
  button_delete_->setEnabled(names.size() > 1);
  updating_ = false;
}
