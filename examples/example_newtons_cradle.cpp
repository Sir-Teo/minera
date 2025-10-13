#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>

using namespace minerva;

/**
 * Example: Newton's Cradle
 *
 * Classic momentum and energy transfer demonstration.
 * A line of touching spheres with one pulled back and released.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  const int n_spheres = 7;
  const double radius = 0.3;
  const double spacing = radius * 2.0; // Touching spheres

  // Create hanging spheres in a line
  for (int i = 0; i < n_spheres; ++i){
    RigidBody rb;
    rb.radius = radius;
    rb.mass = 1.0;
    rb.position = Vec3(-3.0 + i * spacing, 2.0, 0.0);
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  // Pull the first sphere back and give it velocity
  world.rigid_bodies[0].position.x -= 2.0;
  world.rigid_bodies[0].position.y += 0.5; // Slightly higher to simulate pendulum
  world.rigid_bodies[0].velocity = Vec3(5.0, -1.0, 0.0); // Moving toward the others

  // Pull the last sphere back in opposite direction for symmetric effect
  world.rigid_bodies[n_spheres-1].position.x += 2.0;
  world.rigid_bodies[n_spheres-1].position.y += 0.5;
  world.rigid_bodies[n_spheres-1].velocity = Vec3(-5.0, -1.0, 0.0);

  MINERVA_LOG("Newton's Cradle: %zu spheres\n", world.rigid_bodies.size());

  // System with very high restitution for elastic collisions
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.95; // Nearly perfect elastic collision
  rb_cfg.ground_y = 0.0;
  rb_cfg.substeps = 6; // More substeps for accurate collision
  rb_cfg.pair_iterations = 32;
  rb_cfg.penetration_slop = 1e-5; // Tight tolerance

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/cradle";
  csv_cfg.prefix = "cradle";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/cradle";
  vtk_cfg.prefix = "cradle";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/240.0; // Smaller timestep for accuracy
  const int steps = 3600; // 15 seconds
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

    if (s % 240 == 0){
      // Calculate total momentum
      Vec3 total_momentum(0, 0, 0);
      double total_ke = 0.0;
      for (const auto& rb : world.rigid_bodies){
        total_momentum = total_momentum + (rb.velocity * rb.mass);
        total_ke += 0.5 * rb.mass * rb.velocity.norm2();
      }

      std::cout << "t=" << world.time
                << "  px=" << total_momentum.x
                << "  KE=" << total_ke
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/cradle/\n", frame);
  return 0;
}
