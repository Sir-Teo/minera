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
 * Example: Double Helix
 *
 * Spheres arranged in a DNA-like double helix structure that falls and interacts.
 * Two intertwined helical strands with connecting "base pairs" create a visually
 * striking simulation demonstrating complex initial conditions and collision dynamics.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  const double helix_radius = 1.8;
  const double helix_height = 18.0;  // Increased for better vertical spacing
  const double sphere_radius = 0.22;  // Slightly smaller
  const int turns = 4;
  const int spheres_per_turn = 6;  // Reduced to avoid crowding
  const int total_spheres = turns * spheres_per_turn;

  // Create first helix strand (red/left)
  for (int i = 0; i < total_spheres; ++i){
    RigidBody rb;
    rb.radius = sphere_radius;
    rb.mass = 1.0;

    double t = (double)i / spheres_per_turn * 2.0 * M_PI;
    double height = (double)i / total_spheres * helix_height;

    rb.position = Vec3(
      helix_radius * std::cos(t),
      height + 5.0, // Start elevated
      helix_radius * std::sin(t)
    );
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  // Create second helix strand (blue/right) - offset by 180 degrees
  for (int i = 0; i < total_spheres; ++i){
    RigidBody rb;
    rb.radius = sphere_radius;
    rb.mass = 1.0;

    double t = (double)i / spheres_per_turn * 2.0 * M_PI + M_PI;
    double height = (double)i / total_spheres * helix_height;

    rb.position = Vec3(
      helix_radius * std::cos(t),
      height + 5.0,
      helix_radius * std::sin(t)
    );
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  // Add "base pair" connectors between the two strands
  const int connectors = turns * 3; // 3 connectors per turn to avoid overlap
  for (int i = 0; i < connectors; ++i){
    double t = (double)i / connectors * turns * 2.0 * M_PI + 0.3; // Offset to avoid helix spheres
    double height = (double)i / connectors * helix_height + 0.5; // Offset vertically

    // Connector sphere between the two strands
    RigidBody rb;
    rb.radius = sphere_radius * 0.75; // Smaller to avoid overlap
    rb.mass = 0.75;

    // Position at 60% distance from center (not exactly halfway)
    double x1 = helix_radius * std::cos(t);
    double z1 = helix_radius * std::sin(t);
    double x2 = helix_radius * std::cos(t + M_PI);
    double z2 = helix_radius * std::sin(t + M_PI);

    rb.position = Vec3(
      (x1 + x2) * 0.3,  // Closer to center
      height + 5.0,
      (z1 + z2) * 0.3
    );
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  MINERVA_LOG("Double Helix: %zu spheres in DNA-like structure\n", world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

  // System configuration
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.6; // Moderate bounce
  rb_cfg.ground_y = 0.0;
  rb_cfg.substeps = 4;
  rb_cfg.pair_iterations = 24;
  rb_cfg.penetration_slop = 1e-4;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/double_helix";
  csv_cfg.prefix = "double_helix";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/double_helix";
  vtk_cfg.prefix = "double_helix";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/120.0;
  const int steps = 3600; // 30 seconds
  const int output_interval = 4;

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
      double total_ke = 0.0;
      for (const auto& rb : world.rigid_bodies){
        total_ke += 0.5 * rb.mass * rb.velocity.norm2();
      }

      std::cout << "t=" << world.time
                << "  KE=" << total_ke
                << "  bodies=" << world.rigid_bodies.size()
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/double_helix/\n", frame);
  return 0;
}
