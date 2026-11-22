# Fluid solver library: plain C++17 and Eigen, no Qt
project(FluidSolver VERSION 1.0 LANGUAGES CXX)

add_library(fluid_solver
    src/Solver.cpp)

target_include_directories(fluid_solver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(fluid_solver PUBLIC eigen_headers)
