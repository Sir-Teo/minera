#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>

using namespace minerva;

/**
 * Example 1: High Drop
 *
 * Drops a large number of spheres from significant height.
 * Demonstrates ground collision and energy dissipation.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  // Create a tall tower of spheres
  const int layers_x = 8;
  const int layers_y = 15; // Very tall tower
  const int layers_z = 8;
  const double spacing = 0.5; // Radius is 0.2, diameter is 0.4, so 0.5 gives 0.1 clearance

  for (int y=0; y<layers_y; ++y){
    for (int x=0; x<layers_x; ++x){
      for (int z=0; z<layers_z; ++z){
        RigidBody rb;
        rb.radius = 0.2;
        rb.mass   = 1.0;
        rb.position = Vec3(-1.75 + x*spacing, 8.0 + y*spacing, -1.75 + z*spacing);
        // No initial velocity - let gravity accelerate naturally to avoid overlaps
        rb.velocity = Vec3(0, 0, 0);
        world.rigid_bodies.push_back(rb);
      }
    }
  }

  MINERVA_LOG("High Drop: %zu spheres falling from height\n", world.rigid_bodies.size());

  // System with more bouncy restitution
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.7;
  rb_cfg.ground_y = 0.0;
  // Reasonable collision solving - no initial velocity means less aggressive solving needed
  rb_cfg.substeps = 4;
  rb_cfg.pair_iterations = 32;
  rb_cfg.penetration_slop = 1e-4;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/highdrop";
  csv_cfg.prefix = "highdrop";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/highdrop";
  vtk_cfg.prefix = "highdrop";
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
      const auto& rb = world.rigid_bodies.front();
      std::cout << "t=" << world.time
                << "  y=" << rb.position.y
                << "  vy=" << rb.velocity.y
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/highdrop/\n", frame);
  return 0;
}
