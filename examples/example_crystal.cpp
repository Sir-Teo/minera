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
 * Example: Crystal Formation
 *
 * MD particles starting with high temperature, gradually cooling to form
 * an ordered structure. Demonstrates thermodynamic phase transition.
 */
int main(){
  World world;
  world.gravity = Vec3(0, 0, 0); // No gravity

  std::mt19937 rng(789);
  std::uniform_real_distribution<double> pos_dist(-1.5, 1.5);
  std::normal_distribution<double> vel_high(0, std::sqrt(3.0)); // High temperature

  // Create particles in a compact random region
  const int n_particles = 500;

  for (int i = 0; i < n_particles; ++i){
    Particle p;
    p.mass = 1.0;

    // Random starting positions
    p.position = Vec3(
      pos_dist(rng),
      pos_dist(rng),
      pos_dist(rng)
    );

    // High initial velocity (high temperature)
    p.velocity = Vec3(
      vel_high(rng),
      vel_high(rng),
      vel_high(rng)
    );

    world.md_particles.push(p);
  }

  MINERVA_LOG("Crystal Formation: %zu particles cooling from high temperature\n",
              world.md_particles.size());

  // MD configuration with thermostat
  MDConfig md_cfg;
  md_cfg.epsilon = 1.0;
  md_cfg.sigma = 1.0;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.use_neighbor_list = true;
  md_cfg.nvt = true; // Use thermostat
  md_cfg.temp = 0.3; // Target low temperature for crystallization
  md_cfg.tau_thermo = 0.5; // Quick cooling
  md_cfg.nlist_skin = 0.3;

  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/crystal";
  csv_cfg.prefix = "crystal";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/crystal";
  vtk_cfg.prefix = "crystal";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/200.0;
  const int steps = 4000; // 20 seconds
  const int output_interval = 10;

  MINERVA_LOG("Starting simulation: %d steps, dt=%.6f\n", steps, dt);

  int frame = 0;
  for (int s = 0; s < steps; ++s){
    world.step(dt);

    if (s % output_interval == 0) {
      csv_writer->write(world, frame);
      vtk_writer->write(world, frame);
      ++frame;
    }

    if (s % 200 == 0){
      // Calculate instantaneous temperature
      double ke_total = 0.0;
      for (const auto& p : world.md_particles.data){
        ke_total += 0.5 * p.mass * p.velocity.norm2();
      }
      double temp = (2.0/3.0) * ke_total / world.md_particles.size();

      std::cout << "t=" << world.time
                << "  T=" << temp
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/crystal/\n", frame);
  return 0;
}
