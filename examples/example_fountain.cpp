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
 * Example: Fountain
 *
 * Spheres continuously spawned upward from a point, creating a fountain effect.
 * Demonstrates continuous emission and parabolic trajectories.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  // Pre-spawn spheres with upward velocities at different angles
  const int n_jets = 8; // Number of radial jets
  const int particles_per_jet = 15;
  const double launch_speed = 8.0;
  const double angle_spread = 15.0 * M_PI / 180.0; // 15 degree cone

  for (int jet = 0; jet < n_jets; ++jet){
    double base_angle = 2.0 * M_PI * jet / n_jets;

    for (int p = 0; p < particles_per_jet; ++p){
      RigidBody rb;
      rb.radius = 0.15;
      rb.mass = 1.0;

      // Start from center at ground level
      rb.position = Vec3(0, 0.2, 0);

      // Launch angle varies slightly per particle
      double height_angle = M_PI/3.0 + (p - particles_per_jet/2) * 0.05; // ~60 degrees up
      double radial_angle = base_angle + (p - particles_per_jet/2) * 0.1;

      // Velocity in cone pattern
      rb.velocity = Vec3(
        launch_speed * std::cos(height_angle) * std::cos(radial_angle),
        launch_speed * std::sin(height_angle),
        launch_speed * std::cos(height_angle) * std::sin(radial_angle)
      );

      world.rigid_bodies.push_back(rb);
    }
  }

  MINERVA_LOG("Fountain: %zu spheres in fountain pattern\n", world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

  // System with medium restitution
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.6;
  rb_cfg.ground_y = 0.0;
  rb_cfg.substeps = 4;
  rb_cfg.pair_iterations = 24;
  rb_cfg.penetration_slop = 1e-4;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/fountain";
  csv_cfg.prefix = "fountain";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/fountain";
  vtk_cfg.prefix = "fountain";
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
      // Count spheres above ground
      int airborne = 0;
      for (const auto& rb : world.rigid_bodies){
        if (rb.position.y > 0.5) ++airborne;
      }

      std::cout << "t=" << world.time
                << "  airborne=" << airborne
                << "/" << world.rigid_bodies.size()
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/fountain/\n", frame);
  return 0;
}
