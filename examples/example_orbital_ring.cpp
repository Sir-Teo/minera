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
 * Example: Spiral Vortex
 *
 * Spheres arranged in a spiral formation with rotation and inward motion.
 * Creates a mesmerizing spiral galaxy effect as they fall and converge.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -3.0, 0); // Moderate gravity

  const int n_rings = 8;
  const int spheres_per_ring = 16;

  // Create spheres in a 3D spiral
  for (int ring = 0; ring < n_rings; ++ring){
    double radius = 16.0 - ring * 1.6; // Decreasing radius
    double height = 12.0 + ring * 2.5;  // Increasing height
    double phase_offset = ring * 0.3;  // Spiral twist

    for (int i = 0; i < spheres_per_ring; ++i){
      double angle = 2.0 * M_PI * i / spheres_per_ring + phase_offset;

      RigidBody rb;
      rb.radius = 1.0;
      rb.mass = 1.0;

      // Position in spiral
      rb.position = Vec3(
        radius * std::cos(angle),
        height,
        radius * std::sin(angle)
      );

      // Velocity: rotating + falling inward
      double rotation_speed = 2.5;
      double inward_speed = 0.8;
      rb.velocity = Vec3(
        -rotation_speed * std::sin(angle) - inward_speed * std::cos(angle),
        0.0,
        rotation_speed * std::cos(angle) - inward_speed * std::sin(angle)
      );

      world.rigid_bodies.push_back(rb);
    }
  }

  MINERVA_LOG("Spiral Vortex: %zu spheres in spiral formation\n", world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

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
      // Calculate average height and radial distance
      double avg_height = 0.0;
      double avg_radius = 0.0;

      for (const auto& rb : world.rigid_bodies){
        avg_height += rb.position.y;
        avg_radius += std::sqrt(rb.position.x*rb.position.x + rb.position.z*rb.position.z);
      }
      avg_height /= world.rigid_bodies.size();
      avg_radius /= world.rigid_bodies.size();

      std::cout << "t=" << world.time
                << "  avg_height=" << avg_height
                << "  avg_radius=" << avg_radius
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/orbital/\n", frame);
  return 0;
}
