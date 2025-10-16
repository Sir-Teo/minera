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
  simcore/spatial/neighbor_list.hpp
  simcore/spatial/neighbor_list.cpp
  modules/rb/rigid_body_system.hpp
  modules/rb/rigid_body_system.cpp
  modules/md/md_system.hpp
  modules/md/md_system.cpp
)

target_include_directories(minerva_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(minerva_demo examples/minerva_demo.cpp)
target_link_libraries(minerva_demo PRIVATE minerva_core)

add_executable(benchmark_nlist examples/benchmark_nlist.cpp)
target_link_libraries(benchmark_nlist PRIVATE minerva_core)

# Additional examples
add_executable(example_highdrop examples/example_highdrop.cpp)
target_link_libraries(example_highdrop PRIVATE minerva_core)

add_executable(example_md_large examples/example_md_large.cpp)
target_link_libraries(example_md_large PRIVATE minerva_core)

add_executable(example_multiscale examples/example_multiscale.cpp)
target_link_libraries(example_multiscale PRIVATE minerva_core)

add_executable(example_collision examples/example_collision.cpp)
target_link_libraries(example_collision PRIVATE minerva_core)

add_executable(example_avalanche examples/example_avalanche.cpp)
target_link_libraries(example_avalanche PRIVATE minerva_core)

add_executable(example_newtons_cradle examples/example_newtons_cradle.cpp)
target_link_libraries(example_newtons_cradle PRIVATE minerva_core)

add_executable(example_gas_expansion examples/example_gas_expansion.cpp)
target_link_libraries(example_gas_expansion PRIVATE minerva_core)

add_executable(example_orbital_ring examples/example_orbital_ring.cpp)
target_link_libraries(example_orbital_ring PRIVATE minerva_core)

add_executable(example_domino_chain examples/example_domino_chain.cpp)
target_link_libraries(example_domino_chain PRIVATE minerva_core)

add_executable(example_fountain examples/example_fountain.cpp)
target_link_libraries(example_fountain PRIVATE minerva_core)

add_executable(example_vortex examples/example_vortex.cpp)
target_link_libraries(example_vortex PRIVATE minerva_core)

add_executable(example_gravity_well examples/example_gravity_well.cpp)
target_link_libraries(example_gravity_well PRIVATE minerva_core)

add_executable(example_crystal examples/example_crystal.cpp)
target_link_libraries(example_crystal PRIVATE minerva_core)

add_executable(example_wave examples/example_wave.cpp)
target_link_libraries(example_wave PRIVATE minerva_core)

add_executable(example_double_helix examples/example_double_helix.cpp)
target_link_libraries(example_double_helix PRIVATE minerva_core)

add_executable(test_overlaps test_overlaps.cpp)
target_link_libraries(test_overlaps PRIVATE minerva_core)
