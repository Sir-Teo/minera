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
 * Example: Radial Impact Wave
 *
 * Tightly packed grid of spheres on the ground. A projectile strikes
 * one corner creating a radial shockwave that propagates through the grid.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  const int grid_x = 20;
  const int grid_z = 20;
  const double spacing = 2.1; // Tight spacing (diameter is 2.0)

  // Create a tightly packed grid of spheres on the ground
  for (int i = 0; i < grid_x; ++i){
    for (int k = 0; k < grid_z; ++k){
      RigidBody rb;
      rb.radius = 1.0;
      rb.mass = 1.0;

      // Position on ground in tight grid
      rb.position = Vec3(
        -20.0 + i * spacing,
        1.0,  // Just above ground
        -20.0 + k * spacing
      );

      rb.velocity = Vec3(0, 0, 0);
      world.rigid_bodies.push_back(rb);
    }
  }

  // Add a projectile to create impact wave
  RigidBody projectile;
  projectile.radius = 1.5;
  projectile.mass = 10.0; // Heavy projectile
  projectile.position = Vec3(-22.0, 12.0, -22.0); // Above corner
  projectile.velocity = Vec3(20.0, -25.0, 20.0); // Fast diagonal strike
  world.rigid_bodies.push_back(projectile);

  MINERVA_LOG("Radial Impact Wave: %zu spheres + 1 projectile\n",
              world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

  // System with very high restitution for wave propagation
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.9;
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
