#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include "simcore/utils/overlap_checker.hpp"
#include <iostream>
#include <memory>

using namespace minerva;

/**
 * Example: Domino Chain Reaction
 *
 * A line of spheres with the first one given velocity to trigger a cascade.
 * Demonstrates sequential collision propagation.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  const int n_dominos = 30;
  const double spacing = 0.55; // Slightly more than diameter for clean hits

  // Create a line of "dominos" (spheres)
  for (int i = 0; i < n_dominos; ++i){
    RigidBody rb;
    rb.radius = 0.25;
    rb.mass = 1.0;
    rb.position = Vec3(-8.0 + i * spacing, 0.5, 0.0);
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  // Give the first sphere a push
  world.rigid_bodies[0].velocity = Vec3(8.0, 0, 0);

  // Add a second chain parallel to the first
  for (int i = 0; i < n_dominos; ++i){
    RigidBody rb;
    rb.radius = 0.25;
    rb.mass = 1.0;
    rb.position = Vec3(-8.0 + i * spacing, 0.5, 2.0);
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  // Connect the chains with a perpendicular trigger at the end
  for (int i = 0; i < 4; ++i){
    RigidBody rb;
    rb.radius = 0.25;
    rb.mass = 1.0;
    rb.position = Vec3(-8.0 + (n_dominos-1) * spacing, 0.5, 0.5 + i * 0.5);
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  MINERVA_LOG("Domino Chain: %zu spheres in cascade setup\n", world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

  // System with moderate restitution for realistic collisions
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.4; // Some energy loss for realistic domino effect
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
