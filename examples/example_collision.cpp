#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>

using namespace minerva;

/**
 * Example 4: Collision Cascade
 *
 * Two groups of spheres moving towards each other for a dramatic collision.
 * Demonstrates momentum conservation and collision dynamics.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -2.0, 0); // Light gravity

  // Left group - moving right
  const int n_left = 6;
  for (int y=0; y<n_left; ++y){
    for (int z=0; z<n_left; ++z){
      RigidBody rb;
      rb.radius = 0.25;
      rb.mass   = 1.0;
      rb.position = Vec3(-3.0, 0.5 + y*0.55, -1.5 + z*0.55);
      rb.velocity = Vec3(3.0, 0, 0); // Moving right
      world.rigid_bodies.push_back(rb);
    }
  }

  // Right group - moving left
  const int n_right = 6;
  for (int y=0; y<n_right; ++y){
    for (int z=0; z<n_right; ++z){
      RigidBody rb;
      rb.radius = 0.25;
      rb.mass   = 1.0;
      rb.position = Vec3(3.0, 0.5 + y*0.55, -1.5 + z*0.55);
      rb.velocity = Vec3(-3.0, 0, 0); // Moving left
      world.rigid_bodies.push_back(rb);
    }
  }

  // Central stationary group
  const int n_center = 4;
  for (int y=0; y<n_center; ++y){
    for (int z=0; z<n_center; ++z){
      RigidBody rb;
      rb.radius = 0.25;
      rb.mass   = 1.0;
      rb.position = Vec3(0.0, 0.5 + y*0.55, -1.0 + z*0.55);
      rb.velocity = Vec3(0, 0, 0); // Stationary
      world.rigid_bodies.push_back(rb);
    }
  }

  MINERVA_LOG("Collision Cascade: %zu spheres\n", world.rigid_bodies.size());

  // System with medium restitution
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.5;
  rb_cfg.ground_y = 0.0;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/collision";
  csv_cfg.prefix = "collision";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/collision";
  vtk_cfg.prefix = "collision";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/120.0;
  const int steps = 1800; // 15 seconds
  const int output_interval = 6;

  MINERVA_LOG("Starting simulation: %d steps, dt=%.6f\n", steps, dt);

  int frame = 0;
  for (int s=0; s<steps; ++s){
    world.step(dt);

    if (s % output_interval == 0) {
      csv_writer->write(world, frame);
      vtk_writer->write(world, frame);
      ++frame;
    }

    if (s % 120 == 0){
      // Calculate total kinetic energy
      double ke_total = 0.0;
      for (const auto& rb : world.rigid_bodies){
        ke_total += 0.5 * rb.mass * rb.velocity.norm2();
      }
      std::cout << "t=" << world.time
                << "  KE=" << ke_total
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/collision/\n", frame);
  return 0;
}
