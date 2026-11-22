#include "Data.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QSaveFile>

#include <algorithm>

QJsonObject Data::toJson(const Simulation &simulation, const QString &name) {
  const SolverParameters &p = simulation.parameters;
  QJsonObject value;
  value["simulationName"] = name;
  value["viscosity"] = p.viscosity;
  value["timeStep"] = p.target_time_step;
  value["diffusionConstance"] = p.diffusion;
  value["flowSpeed"] = p.flow_speed;
  value["rowSize"] = static_cast<int>(p.width);
  value["colSize"] = static_cast<int>(p.height);
  value["linearSolverIterration"] = static_cast<int>(p.solver_iterations);
  value["max_speed_calculation"] = static_cast<int>(p.scalar_interval);
  value["force"] = p.force;
  QString obstacles;
  for (const QPoint &point : simulation.obstacles) {
    obstacles += QString("%1,%2;").arg(point.x()).arg(point.y());
  }
  value["obstacle_points"] = obstacles;
  return value;
}

Simulation Data::fromJson(const QJsonObject &object) {
  Simulation simulation;
  SolverParameters &p = simulation.parameters;
  auto grid = [](const QJsonValue &value, std::size_t fallback) {
    return static_cast<std::size_t>(std::clamp(value.toInt(static_cast<int>(fallback)), MIN_GRID_SIZE, MAX_GRID_SIZE));
  };
  p.viscosity = std::max(0.0f, static_cast<float>(object["viscosity"].toDouble(p.viscosity)));
  p.target_time_step = std::max(0.0001f, static_cast<float>(object["timeStep"].toDouble(p.target_time_step)));
  p.diffusion = std::max(0.0f, static_cast<float>(object["diffusionConstance"].toDouble(p.diffusion)));
  p.flow_speed = static_cast<float>(object["flowSpeed"].toDouble(p.flow_speed));
  p.width = grid(object["rowSize"], p.width);
  p.height = grid(object["colSize"], p.height);
  p.solver_iterations = static_cast<std::size_t>(std::max(1, object["linearSolverIterration"].toInt(20)));
  p.scalar_interval = static_cast<std::size_t>(std::max(1, object["max_speed_calculation"].toInt(1)));
  p.force = static_cast<float>(object["force"].toDouble(p.force));

  const QStringList points = object["obstacle_points"].toString().split(';', Qt::SkipEmptyParts);
  for (const QString &point : points) {
    QStringList coordinates = point.split(',');
    bool ok_x = false;
    bool ok_y = false;
    if (coordinates.size() != 2) {
      continue;
    }
    QPoint cell(coordinates[0].toInt(&ok_x), coordinates[1].toInt(&ok_y));
    if (ok_x && ok_y && !simulation.obstacles.contains(cell)) {
      simulation.obstacles.append(cell);
    }
  }
  return simulation;
}

bool Data::loadFromFile(const QString &file_path) {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  QMap<QString, Simulation> simulations;
  const QJsonObject document = QJsonDocument::fromJson(file.readAll()).object();
  const QJsonArray root = document["simulations"].toArray();
  for (const QJsonValue &value : root) {
    QJsonObject object = value.toObject();
    QString name = object["simulationName"].toString().trimmed();
    if (!name.isEmpty() && !simulations.contains(name)) {
      simulations.insert(name, fromJson(object));
    }
  }
  if (simulations.isEmpty()) {
    return false;
  }
  simulations_ = simulations;
  current_name_ = simulations_.firstKey();
  preset_version_ = document["presetVersion"].toInt(1);
  return true;
}

bool Data::storeToFile(const QString &file_path) const {
  QJsonArray root;
  for (auto iter = simulations_.cbegin(); iter != simulations_.cend(); ++iter) {
    root.append(toJson(iter.value(), iter.key()));
  }
  QJsonObject master;
  master["presetVersion"] = preset_version_;
  master["simulations"] = root;

  QDir().mkpath(QFileInfo(file_path).absolutePath());
  QSaveFile file(file_path);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  file.write(QJsonDocument(master).toJson());
  return file.commit();
}

void Data::addPresets(const Data &presets) {
  for (auto iter = presets.simulations_.cbegin(); iter != presets.simulations_.cend(); ++iter) {
    if (!simulations_.contains(iter.key())) {
      simulations_.insert(iter.key(), iter.value());
    }
  }
  preset_version_ = presets.preset_version_;
}

Simulation &Data::current() {
  if (simulations_.isEmpty()) {
    current_name_ = QStringLiteral("Default");
    simulations_.insert(current_name_, Simulation());
  }
  return simulations_[current_name_];
}

bool Data::select(const QString &name) {
  if (!simulations_.contains(name)) {
    return false;
  }
  current_name_ = name;
  return true;
}

QString Data::duplicateCurrent() {
  QString name = QStringLiteral("Untitled");
  for (int i = 2; simulations_.contains(name); i++) {
    name = QStringLiteral("Untitled %1").arg(i);
  }
  Simulation copy = current();
  simulations_.insert(name, copy);
  current_name_ = name;
  return name;
}

bool Data::rename(const QString &old_name, const QString &new_name) {
  QString name = new_name.trimmed();
  if (name.isEmpty() || simulations_.contains(name) || !simulations_.contains(old_name)) {
    return false;
  }
  simulations_.insert(name, simulations_.take(old_name));
  if (current_name_ == old_name) {
    current_name_ = name;
  }
  return true;
}

bool Data::remove(const QString &name) {
  if (simulations_.size() <= 1 || !simulations_.contains(name)) {
    return false;
  }
  if (name == current_name_) {
    QStringList all = names();
    qsizetype index = all.indexOf(name);
    current_name_ = all.at(index > 0 ? index - 1 : index + 1);
  }
  simulations_.remove(name);
  return true;
}
