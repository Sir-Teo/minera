#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include "simcore/utils/overlap_checker.hpp"
#include <iostream>
#include <memory>
#include <random>
#include <cmath>

using namespace minerva;

/**
 * Example: Avalanche
 *
 * Spheres tumbling down a slope, simulating an avalanche effect.
 * Uses angled initial positions and velocities to simulate terrain.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  std::mt19937 rng(2024);
  std::uniform_real_distribution<double> jitter(-0.02, 0.02); // Reduced jitter

  // Create a "slope" by placing spheres at various heights
  // and giving them a slight downhill velocity
  const double slope_angle = 25.0 * M_PI / 180.0; // 25 degree slope
  const int layers = 8;
  const int width = 10;
  const double spacing = 0.5; // Increased spacing (was 0.45, radius is 0.2)

  for (int layer = 0; layer < layers; ++layer){
    // Each layer is progressively higher and further back
    double base_x = -2.0 - layer * 1.5;
    double base_y = 3.0 + layer * 1.2;

    for (int i = 0; i < width; ++i){
      for (int j = 0; j < width; ++j){
        RigidBody rb;
        rb.radius = 0.2;
        rb.mass = 1.0;

        // Position with jitter to make it more natural
        rb.position = Vec3(
          base_x + j * spacing + jitter(rng),
          base_y + i * spacing + jitter(rng) * 0.5,
          -2.0 + i * spacing + jitter(rng)
        );

        // Give initial "downhill" velocity that increases with layer
        double speed = 0.5 + layer * 0.3;
        rb.velocity = Vec3(
          speed * std::cos(slope_angle) + jitter(rng) * 0.2,
          -speed * std::sin(slope_angle) * 0.5,
          jitter(rng) * 0.2
        );

        world.rigid_bodies.push_back(rb);
      }
    }
  }

  MINERVA_LOG("Avalanche: %zu spheres tumbling down slope\n", world.rigid_bodies.size());

  // Check and resolve initial overlaps
  resolve_initial_overlaps(world, 50);
  check_rigid_body_overlaps(world);

  // System with low restitution for inelastic tumbling
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.3; // Low bounce for realistic avalanche
  rb_cfg.ground_y = 0.0;
  // Use improved defaults (substeps=4, pair_iterations=32 from config)

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/avalanche";
  csv_cfg.prefix = "avalanche";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/avalanche";
  vtk_cfg.prefix = "avalanche";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/120.0;
  const int steps = 2400; // 20 seconds
  const int output_interval = 8;

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
      // Calculate center of mass
      Vec3 com(0, 0, 0);
      for (const auto& rb : world.rigid_bodies){
        com = com + rb.position;
      }
      com = com * (1.0 / world.rigid_bodies.size());

      std::cout << "t=" << world.time
                << "  COM=(" << com.x << "," << com.y << "," << com.z << ")"
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/avalanche/\n", frame);
  return 0;
}
