#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>
#include <cmath>

using namespace minerva;

/**
 * Example: Orbital Ring
 *
 * Spheres arranged in a ring with tangential velocities.
 * Demonstrates centripetal motion and orbital mechanics.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -2.0, 0); // Light gravity to keep it interesting

  const int n_spheres = 24; // Spheres around the ring
  const double orbit_radius = 4.0;
  const double orbital_speed = 3.0;

  // Create spheres in a horizontal ring with tangential velocities
  for (int i = 0; i < n_spheres; ++i){
    double angle = 2.0 * M_PI * i / n_spheres;

    RigidBody rb;
    rb.radius = 0.2;
    rb.mass = 1.0;

    // Position on circle
    rb.position = Vec3(
      orbit_radius * std::cos(angle),
      3.0, // Height above ground
      orbit_radius * std::sin(angle)
    );

    // Tangential velocity (perpendicular to radius)
    rb.velocity = Vec3(
      -orbital_speed * std::sin(angle),
      0.0,
      orbital_speed * std::cos(angle)
    );

    world.rigid_bodies.push_back(rb);
  }

  // Add a central "planet" sphere at the origin (kinematic)
  RigidBody planet;
  planet.radius = 0.8;
  planet.mass = 100.0;
  planet.position = Vec3(0, 3.0, 0);
  planet.velocity = Vec3(0, 0, 0);
  planet.kinematic = true; // Fixed in place
  world.rigid_bodies.push_back(planet);

  MINERVA_LOG("Orbital Ring: %zu spheres in orbital motion\n", world.rigid_bodies.size());

  // System with high restitution
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.8;
  rb_cfg.ground_y = 0.0;
  rb_cfg.substeps = 4;
  rb_cfg.pair_iterations = 24;
  rb_cfg.penetration_slop = 1e-4;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/orbital";
  csv_cfg.prefix = "orbital";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/orbital";
  vtk_cfg.prefix = "orbital";
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
      // Calculate average orbital radius (excluding planet)
      double avg_radius = 0.0;
      const Vec3 center(0, 3.0, 0);

      for (size_t i = 0; i < world.rigid_bodies.size() - 1; ++i){ // Exclude planet
        Vec3 r = world.rigid_bodies[i].position - center;
        avg_radius += std::sqrt(r.x*r.x + r.z*r.z); // Horizontal distance only
      }
      avg_radius /= (world.rigid_bodies.size() - 1);

      std::cout << "t=" << world.time
                << "  avg_radius=" << avg_radius
                << "  delta=" << (avg_radius - orbit_radius)
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/orbital/\n", frame);
  return 0;
}
