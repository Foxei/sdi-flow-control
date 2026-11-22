#ifndef FLUID_SOLVER_SOLVER_PARAMETERS_H
#define FLUID_SOLVER_SOLVER_PARAMETERS_H

#include <cstddef>

/**
 * All parameters of a simulation that the user can change. The grid size is
 * only applied when a solver is created.
 */
struct SolverParameters {
  std::size_t width = 80;             //< Number of cells in flow direction (x)
  std::size_t height = 40;            //< Number of cells across the channel (y)
  float viscosity = 1;                //< Viscosity of the fluid
  float diffusion = 0;                //< Diffusivity of the ink
  float flow_speed = 1;               //< Speed of the fluid entering the channel
  float target_time_step = 1;         //< Simulated time per second of real time
  float time_step = 0.03f;            //< Time step of the next solver step
  std::size_t solver_iterations = 20; //< Gauss-Seidel iterations per linear solve
  std::size_t scalar_interval = 1;    //< Steps between updates of the maximum values
  float force = 5;                    //< Force applied to a cell by a mouse drag
};

/**
 * Boundary handling of a field
 */
enum class FieldDirection {
  X,    //< Velocity in x direction: inflow on the left, slip walls
  Y,    //< Velocity in y direction: no flow through the walls
  NONE, //< Scalar field: zero gradient at all boundaries
  ZERO  //< Scalar field: zero at all boundaries
};

#endif //FLUID_SOLVER_SOLVER_PARAMETERS_H
