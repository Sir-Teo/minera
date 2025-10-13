#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "modules/md/md_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>
#include <random>

using namespace minerva;

/**
 * Example 3: Multi-Scale Interaction
 *
 * Demonstrates both rigid body and molecular dynamics in one simulation.
 * Shows the power of the modular scheduler system.
 */
int main(){
  World world;
  world.gravity = Vec3(0, -5.0, 0); // Moderate gravity

  // --- Part 1: Rigid body "containers" falling
  {
    // Create a few large spheres falling down
    for (int i=0; i<4; ++i){
      for (int j=0; j<4; ++j){
        RigidBody rb;
        rb.radius = 0.3;
        rb.mass   = 2.0;
        rb.position = Vec3(-1.5 + i*1.0, 6.0 + j*1.2, 0.0);
        rb.velocity = Vec3(0, -1.0, 0);
        world.rigid_bodies.push_back(rb);
      }
    }
    MINERVA_LOG("Multi-scale: %zu rigid bodies\n", world.rigid_bodies.size());
  }

  // --- Part 2: MD particle cloud
  {
    std::mt19937 rng(999);
    std::uniform_real_distribution<double> pos_x(-2.0, 2.0);
    std::uniform_real_distribution<double> pos_y(0.5, 3.0);
    std::uniform_real_distribution<double> pos_z(-2.0, 2.0);
    std::normal_distribution<double> vel(0.0, std::sqrt(1.0));

    // Create a cloud of 300 particles
    for (int i=0; i<300; ++i){
      Particle p;
      p.mass = 1.0;
      p.position = Vec3(pos_x(rng), pos_y(rng), pos_z(rng));
      p.velocity = Vec3(vel(rng), vel(rng), vel(rng));
      world.md_particles.push(p);
    }
    MINERVA_LOG("Multi-scale: %zu MD particles\n", world.md_particles.size());
  }

  // --- Systems (both active simultaneously)
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.6;
  rb_cfg.ground_y = 0.0;

  MDConfig md_cfg;
  md_cfg.epsilon = 0.8;
  md_cfg.sigma = 1.0;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.use_neighbor_list = false;  // Only 300 particles, all-pairs is fine
  md_cfg.nvt = true;
  md_cfg.temp = 1.2;
  md_cfg.tau_thermo = 1.0;

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);
  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/multiscale";
  csv_cfg.prefix = "multiscale";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/multiscale";
  vtk_cfg.prefix = "multiscale";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/150.0;
  const int steps = 3000;  // 20 seconds
  const int output_interval = 10;

  MINERVA_LOG("Starting multi-scale simulation: %d steps, dt=%.6f\n", steps, dt);

  int frame = 0;
  for (int s=0; s<steps; ++s){
    world.step(dt);

    if (s % output_interval == 0) {
      csv_writer->write(world, frame);
      vtk_writer->write(world, frame);
      ++frame;
    }

    if (s % 150 == 0){
      const auto& rb = world.rigid_bodies.front();

      // Calculate MD temperature
      double ke_total = 0.0;
      for (const auto& p : world.md_particles.data){
        ke_total += 0.5 * p.mass * p.velocity.norm2();
      }
      double temp = world.md_particles.size() > 0 ?
                    (2.0/3.0) * ke_total / world.md_particles.size() : 0.0;

      std::cout << "t=" << world.time
                << "  RB.y=" << rb.position.y
                << "  MD.T=" << temp
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/multiscale/\n", frame);
  return 0;
}
