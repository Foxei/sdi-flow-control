#ifndef REAL_TIME_FLUID_SIMULATION_LIST_H
#define REAL_TIME_FLUID_SIMULATION_LIST_H

#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

/**
 * List of the stored simulations with buttons to duplicate and delete them.
 * A double click on a name renames it.
 */
class SimulationList : public QWidget {
 Q_OBJECT
 private:
  QListWidget *list_ = nullptr;
  QPushButton *button_duplicate_ = nullptr;
  QPushButton *button_delete_ = nullptr;
  QStringList names_;     //< Names in the list, to find the old name on rename
  bool updating_ = false; //< Ignore signals while the list is refilled

  void setUpGui();

 public:
  explicit SimulationList(QWidget *parent = nullptr);

 public slots:
  /**
   * Fill the list and select the active simulation.
   */
  void setSimulations(const QStringList &names, const QString &current);

 signals:
  void simulationSelected(const QString &name);
  void duplicateClicked();
  void deleteClicked(const QString &name);
  void simulationRenamed(const QString &old_name, const QString &new_name);
};

#endif //REAL_TIME_FLUID_SIMULATION_LIST_H
