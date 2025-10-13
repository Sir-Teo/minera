#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>
#include <random>
#include <cmath>

using namespace minerva;

/**
 * Example: Gravity Well
 *
 * Spheres rolling into a central depression, like marbles in a funnel.
 * Demonstrates radial motion and energy dissipation.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -5.0, 0); // Moderate gravity

  std::mt19937 rng(456);
  std::uniform_real_distribution<double> radius_dist(4.0, 8.0);
  std::uniform_real_distribution<double> angle_dist(0, 2.0 * M_PI);
  std::normal_distribution<double> vel_noise(0, 0.5);

  // Create spheres in a ring around the center
  const int n_spheres = 100;

  for (int i = 0; i < n_spheres; ++i){
    RigidBody rb;
    rb.radius = 0.2;
    rb.mass = 1.0;

    double r = radius_dist(rng);
    double theta = angle_dist(rng);

    // Position in a ring, elevated
    rb.position = Vec3(
      r * std::cos(theta),
      3.0, // Starting height
      r * std::sin(theta)
    );

    // Small initial velocity toward center plus noise
    double v_radial = -1.5;
    rb.velocity = Vec3(
      v_radial * std::cos(theta) + vel_noise(rng),
      vel_noise(rng) * 0.5,
      v_radial * std::sin(theta) + vel_noise(rng)
    );

    world.rigid_bodies.push_back(rb);
  }

  MINERVA_LOG("Gravity Well: %zu spheres converging to center\n", world.rigid_bodies.size());

  // System with low restitution for energy loss
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.3;
  rb_cfg.ground_y = 0.0;
  rb_cfg.substeps = 4;
  rb_cfg.pair_iterations = 24;
  rb_cfg.penetration_slop = 1e-4;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/gravity_well";
  csv_cfg.prefix = "gravity_well";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/gravity_well";
  vtk_cfg.prefix = "gravity_well";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/120.0;
  const int steps = 2400; // 20 seconds
  const int output_interval = 8;

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
      // Calculate average distance from center
      double avg_r = 0.0;
      for (const auto& rb : world.rigid_bodies){
        avg_r += std::sqrt(rb.position.x * rb.position.x +
                          rb.position.z * rb.position.z);
      }
      avg_r /= world.rigid_bodies.size();

      std::cout << "t=" << world.time
                << "  avg_radius=" << avg_r
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/gravity_well/\n", frame);
  return 0;
}
