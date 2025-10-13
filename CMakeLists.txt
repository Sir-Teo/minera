cmake_minimum_required(VERSION 3.20)
project(minerva LANGUAGES CXX)

option(MINERVA_ENABLE_SANITIZERS "Enable ASan/UBSan" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (APPLE)
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -fno-exceptions)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if (MINERVA_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

add_library(minerva_core
  simcore/base/log.hpp
  simcore/math/vec3.hpp
  simcore/state/rigid_body.hpp
  simcore/state/particle.hpp
  simcore/scheduler/scheduler.hpp
  simcore/world.hpp
  simcore/io/writer.hpp
  simcore/io/csv_writer.hpp
  simcore/io/csv_writer.cpp
  simcore/io/vtk_writer.hpp
  simcore/io/vtk_writer.cpp
  modules/rb/rigid_body_system.hpp
  modules/rb/rigid_body_system.cpp
  modules/md/md_system.hpp
  modules/md/md_system.cpp
)

target_include_directories(minerva_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(minerva_demo examples/minerva_demo.cpp)
target_link_libraries(minerva_demo PRIVATE minerva_core)
