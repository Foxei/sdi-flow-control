cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(RealTimeFluid VERSION 1.0 LANGUAGES CXX)

# Set CMake flag to work with Qt
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt 6 requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# Eigen (header only). The dev container provides it; cross builds pass
# -DEigen3_DIR, plain source trees can be used with -DEIGEN3_INCLUDE_DIR.
set(EIGEN3_INCLUDE_DIR "" CACHE PATH "Eigen source directory (if no Eigen3Config.cmake is available)")
add_library(eigen_headers INTERFACE)
if(EIGEN3_INCLUDE_DIR)
  target_include_directories(eigen_headers SYSTEM INTERFACE ${EIGEN3_INCLUDE_DIR})
else()
  find_package(Eigen3 3.4 REQUIRED NO_MODULE)
  target_link_libraries(eigen_headers INTERFACE Eigen3::Eigen)
endif()

# Finding Qt
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Svg)
message(STATUS "Qt6 version: ${Qt6_VERSION} ")

# Set resources for the application
set(QT_RES ${CMAKE_CURRENT_SOURCE_DIR}/res/res.qrc)

# Targets that we develop here
add_subdirectory(src)
