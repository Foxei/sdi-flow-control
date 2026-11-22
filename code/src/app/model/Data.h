#ifndef REAL_TIME_FLUID_DATA_H
#define REAL_TIME_FLUID_DATA_H

#include "model/Definitions.h"

#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * All stored simulations and the active one. Changes to the active
 * simulation are stored directly. Lives in the worker thread.
 */
class Data {
 private:
  QMap<QString, Simulation> simulations_; //< Stored simulations by name
  QString current_name_;                  //< Name of the active simulation
  int preset_version_ = 1;                //< Version of the presets these simulations contain

  /**
   * Convert a simulation to a json node (keys compatible with the 2017 format).
   */
  static QJsonObject toJson(const Simulation &simulation, const QString &name);

  /**
   * Convert a json node to a simulation, invalid values are clamped.
   */
  static Simulation fromJson(const QJsonObject &object);

 public:
  /** Smallest and largest allowed grid size */
  static constexpr int MIN_GRID_SIZE = 10;
  static constexpr int MAX_GRID_SIZE = 400;

  /**
   * Load simulations from a json file, replaces all stored simulations.
   * @param file_path file to load, may be a Qt resource
   * @return false if the file does not exist or contains no simulation
   */
  bool loadFromFile(const QString &file_path);

  /**
   * Store all simulations to a json file.
   * @param file_path file to write, its directory is created
   * @return true if successful
   */
  bool storeToFile(const QString &file_path) const;

  /**
   * @return version of the presets these simulations contain, 1 for files
   *         without a version (2017 format)
   */
  int presetVersion() const { return preset_version_; }

  /**
   * Add all simulations of the presets whose names are not taken and take
   * over the preset version. Used to add new presets to the simulations of
   * the user without bringing back deleted ones.
   * @param presets simulations shipped with the app
   */
  void addPresets(const Data &presets);

  /**
   * @return the active simulation
   */
  Simulation &current();

  /**
   * @return name of the active simulation
   */
  QString currentName() const { return current_name_; }

  /**
   * @return names of all stored simulations, sorted
   */
  QStringList names() const { return simulations_.keys(); }

  /**
   * @return true if a simulation with this name exists
   */
  bool contains(const QString &name) const { return simulations_.contains(name); }

  /**
   * Make a stored simulation the active one.
   * @return false if it does not exist
   */
  bool select(const QString &name);

  /**
   * Store a copy of the active simulation under a new name and activate it.
   * @return name of the copy
   */
  QString duplicateCurrent();

  /**
   * Rename a stored simulation.
   * @return false if the new name is empty or taken
   */
  bool rename(const QString &old_name, const QString &new_name);

  /**
   * Remove a stored simulation. The last simulation can not be removed. If
   * the active simulation is removed, its neighbour becomes active.
   * @return false if nothing was removed
   */
  bool remove(const QString &name);
};

#endif //REAL_TIME_FLUID_DATA_H
