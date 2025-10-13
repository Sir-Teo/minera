#include "simcore/world.hpp"
#include "modules/md/md_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>
#include <random>

using namespace minerva;

/**
 * Example 2: Large MD System
 *
 * Large-scale molecular dynamics simulation with 512 particles.
 * Demonstrates neighbor list performance and thermal equilibration.
 */
int main(){
  World world;
  world.gravity = Vec3(0, 0, 0); // No gravity for MD

  // Create a large 8x8x8 grid of particles
  const int n_side = 8;
  const double spacing = 1.3;
  const double temp_init = 2.0; // Initial temperature for random velocities

  std::mt19937 rng(42);
  std::normal_distribution<double> vel_dist(0.0, std::sqrt(temp_init));

  for (int i=0; i<n_side; ++i){
    for (int j=0; j<n_side; ++j){
      for (int k=0; k<n_side; ++k){
        Particle p;
        p.mass = 1.0;
        p.position = Vec3(i*spacing, j*spacing, k*spacing);
        // Random thermal velocities
        p.velocity = Vec3(vel_dist(rng), vel_dist(rng), vel_dist(rng));
        world.md_particles.push(p);
      }
    }
  }

  MINERVA_LOG("Large MD: %zu particles with neighbor lists\n", world.md_particles.size());

  // MD system with neighbor lists enabled
  MDConfig md_cfg;
  md_cfg.epsilon = 1.0;
  md_cfg.sigma = 1.0;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.use_neighbor_list = true;
  md_cfg.nlist_skin = 0.3;
  md_cfg.nlist_check_interval = 10;
  md_cfg.nvt = true;          // Use thermostat
  md_cfg.temp = 1.5;          // Target temperature
  md_cfg.tau_thermo = 1.0;    // Thermostat coupling time

  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/md_large";
  csv_cfg.prefix = "md_large";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/md_large";
  vtk_cfg.prefix = "md_large";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/200.0; // Smaller timestep for MD
  const int steps = 3000;      // 15 seconds
  const int output_interval = 10;

  MINERVA_LOG("Starting simulation: %d steps, dt=%.6f\n", steps, dt);

  int frame = 0;
  for (int s=0; s<steps; ++s){
    world.step(dt);

    if (s % output_interval == 0) {
      csv_writer->write(world, frame);
      vtk_writer->write(world, frame);
      ++frame;
    }

    if (s % 200 == 0){
      // Calculate kinetic temperature
      double ke_total = 0.0;
      for (const auto& p : world.md_particles.data){
        ke_total += 0.5 * p.mass * p.velocity.norm2();
      }
      double temp = (2.0/3.0) * ke_total / world.md_particles.size();

      std::cout << "t=" << world.time
                << "  T=" << temp
                << "  N=" << world.md_particles.size()
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/md_large/\n", frame);
  return 0;
}
