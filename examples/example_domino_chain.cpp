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
 * Example: Domino Chain Reaction
 *
 * Tightly packed spheres on the ground forming a curved path.
 * A projectile strikes the first one to trigger a beautiful cascade effect.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  const int n_dominos = 40;
  const double spacing = 0.405; // Ultra-tight spacing - nearly touching (diameter = 0.4)

  // Create a serpentine/curved path of spheres on the ground
  for (int i = 0; i < n_dominos; ++i){
    RigidBody rb;
    rb.radius = 0.2;
    rb.mass = 1.0;

    // Serpentine path: sine wave pattern
    double t = static_cast<double>(i) / 8.0;
    double x = -5.0 + i * spacing;
    double z = 2.5 * std::sin(t);

    rb.position = Vec3(x, 0.5, z); // Elevated to reduce ground friction
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  // Add a heavy projectile to strike the first domino
  RigidBody projectile;
  projectile.radius = 0.3;
  projectile.mass = 4.0;
  projectile.position = Vec3(-7.0, 0.5, 0.0); // Same height as dominoes
  projectile.velocity = Vec3(25.0, 0, 0); // Very fast horizontal strike
  world.rigid_bodies.push_back(projectile);

  MINERVA_LOG("Domino Chain: %zu spheres + projectile in serpentine path\n", world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

  // System with very high restitution for maximum momentum transfer
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.95; // Near-elastic collisions for chain reaction
  rb_cfg.ground_y = 0.0;
  rb_cfg.substeps = 4;
  rb_cfg.pair_iterations = 24;
  rb_cfg.penetration_slop = 1e-4;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/domino";
  csv_cfg.prefix = "domino";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/domino";
  vtk_cfg.prefix = "domino";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/120.0;
  const int steps = 2400; // 20 seconds
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
      // Count how many spheres are moving (velocity magnitude > threshold)
      int moving_count = 0;
      for (const auto& rb : world.rigid_bodies){
        if (rb.velocity.norm2() > 0.1) {
          ++moving_count;
        }
      }

      std::cout << "t=" << world.time
                << "  moving=" << moving_count
                << "/" << world.rigid_bodies.size()
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/domino/\n", frame);
  return 0;
}
