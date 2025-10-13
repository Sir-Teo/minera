#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include "simcore/utils/overlap_checker.hpp"
#include <iostream>
#include <memory>
#include <cmath>

using namespace minerva;

/**
 * Example: Wave Propagation
 *
 * Grid of spheres with one edge given initial displacement, creating
 * a wave that propagates through the grid via collisions.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -5.0, 0);

  const int grid_x = 15;
  const int grid_z = 15;
  const double spacing = 0.5;

  // Create a grid of spheres
  for (int i = 0; i < grid_x; ++i){
    for (int k = 0; k < grid_z; ++k){
      RigidBody rb;
      rb.radius = 0.2;
      rb.mass = 1.0;

      // Position in grid
      rb.position = Vec3(
        -3.5 + i * spacing,
        1.0,
        -3.5 + k * spacing
      );

      // Give edge a wave-like initial velocity
      if (i == 0){
        // Sinusoidal initial velocity along one edge
        double phase = 2.0 * M_PI * k / grid_z;
        rb.velocity = Vec3(
          3.0 * std::sin(phase),
          0,
          0
        );
      } else {
        rb.velocity = Vec3(0, 0, 0);
      }

      world.rigid_bodies.push_back(rb);
    }
  }

  MINERVA_LOG("Wave Propagation: %zu spheres in grid pattern\n",
              world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

  // System with high restitution for wave propagation
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.8;
  rb_cfg.ground_y = 0.0;
  rb_cfg.substeps = 4;
  rb_cfg.pair_iterations = 24;
  rb_cfg.penetration_slop = 1e-4;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/wave";
  csv_cfg.prefix = "wave";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/wave";
  vtk_cfg.prefix = "wave";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/120.0;
  const int steps = 1800; // 15 seconds
  const int output_interval = 6;

  MINERVA_LOG("Starting simulation: %d steps, dt=%.6f\n", steps, dt);

  int frame = 0;
  for (int s = 0; s < steps; ++s){
    world.step(dt);

    if (s % output_interval == 0) {
      csv_writer->write(world, frame);
      vtk_writer->write(world, frame);
      ++frame;
    }

    if (s % 120 == 0){
      // Calculate total kinetic energy
      double ke = 0.0;
      for (const auto& rb : world.rigid_bodies){
        ke += 0.5 * rb.mass * rb.velocity.norm2();
      }

      std::cout << "t=" << world.time
                << "  KE=" << ke
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/wave/\n", frame);
  return 0;
}
