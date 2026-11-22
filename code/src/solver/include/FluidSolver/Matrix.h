#ifndef FLUID_SOLVER_MATRIX_H
#define FLUID_SOLVER_MATRIX_H

#include <Eigen/Dense>

#include <cstddef>
#include <utility>

/**
 * A field on the grid, including one layer of ghost cells on each side.
 */
class Matrix {
 private:
  Eigen::MatrixXf matrix_; //< Values, initialised with zero

 public:
  /**
   * Allocates a zero matrix.
   * @param m row count
   * @param n column count
   */
  Matrix(std::size_t m, std::size_t n);

  /**
   * Return the Eigen matrix to work on.
   * @return pointer to the Eigen matrix
   */
  Eigen::MatrixXf *matrix() { return &matrix_; }
};

/**
 * Two matrices for the current and the previous step. Swapping is O(1).
 */
class SingleStepMatrix {
 private:
  Eigen::MatrixXf current_;  //< Storage of the current step
  Eigen::MatrixXf previous_; //< Storage of the previous step

 public:
  /**
   * Allocates two zero matrices.
   * @param m row count
   * @param n column count
   */
  SingleStepMatrix(std::size_t m, std::size_t n);

  /**
   * @return matrix of the current step
   */
  Eigen::MatrixXf *current() { return &current_; }

  /**
   * @return matrix of the previous step
   */
  Eigen::MatrixXf *previous() { return &previous_; }

  /**
   * Swaps current and previous step.
   */
  void swap() { current_.swap(previous_); }
};

#endif //FLUID_SOLVER_MATRIX_H
