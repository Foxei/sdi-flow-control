#ifndef FLUID_SOLVER_TRAJECTORY_POINT_H
#define FLUID_SOLVER_TRAJECTORY_POINT_H

#include <Eigen/Dense>

#include <vector>

/**
 * A traced particle: its current position and all previous positions.
 */
class TrajectoryPoint {
 private:
  std::vector<Eigen::Vector2f> all_points_; //< Current and all older positions

 public:
  /**
   * Create a path that starts at the given point.
   * @param start_point first position
   */
  explicit TrajectoryPoint(const Eigen::Vector2f &start_point) { all_points_.push_back(start_point); }

  /**
   * @return last point of the path
   */
  const Eigen::Vector2f &getCurrentPoint() const { return all_points_.back(); }

  /**
   * Append a new current point to the path.
   * @param new_point new current point
   */
  void appendPoint(const Eigen::Vector2f &new_point) { all_points_.push_back(new_point); }

  /**
   * @return the whole path
   */
  const std::vector<Eigen::Vector2f> &getList() const { return all_points_; }
};

#endif //FLUID_SOLVER_TRAJECTORY_POINT_H
