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
 * Example: Gas Expansion
 *
 * Molecular dynamics simulation of gas expansion from a compressed state.
 * Demonstrates thermodynamic expansion and particle diffusion.
 */
int main(){
  World world;
  world.gravity = Vec3(0, 0, 0); // No gravity for gas simulation

  std::mt19937 rng(42);
  std::normal_distribution<double> vel(0.0, std::sqrt(2.0)); // High temperature

  // Create a dense cluster of particles (compressed gas)
  const int n_side = 10; // 10x10x10 = 1000 particles
  const double spacing = 0.9; // Very tight spacing

  for (int i = 0; i < n_side; ++i){
    for (int j = 0; j < n_side; ++j){
      for (int k = 0; k < n_side; ++k){
        Particle p;
        p.mass = 1.0;
        // Start in a compact cube at the origin
        p.position = Vec3(
          -2.25 + i * spacing,
          -2.25 + j * spacing,
          -2.25 + k * spacing
        );
        // High random velocities for thermal motion
        p.velocity = Vec3(vel(rng), vel(rng), vel(rng));
        world.md_particles.push(p);
      }
    }
  }

  MINERVA_LOG("Gas Expansion: %zu particles expanding from compressed state\n",
              world.md_particles.size());

  // MD configuration for gas simulation
  MDConfig md_cfg;
  md_cfg.epsilon = 0.5;
  md_cfg.sigma = 1.0;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.use_neighbor_list = true; // Important for 1000 particles
  md_cfg.nvt = false; // NVE - let energy be conserved, watch expansion
  md_cfg.nlist_skin = 0.3;

  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/gas";
  csv_cfg.prefix = "gas";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/gas";
  vtk_cfg.prefix = "gas";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/200.0; // Small timestep for MD
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
      // Calculate center of mass and spread
      Vec3 com(0, 0, 0);
      double total_ke = 0.0;

      for (const auto& p : world.md_particles.data){
        com = com + p.position;
        total_ke += 0.5 * p.mass * p.velocity.norm2();
      }

      com = com * (1.0 / world.md_particles.size());

      // Calculate RMS distance from COM (measure of spread)
      double rms_dist = 0.0;
      for (const auto& p : world.md_particles.data){
        Vec3 r = p.position - com;
        rms_dist += r.norm2();
      }
      rms_dist = std::sqrt(rms_dist / world.md_particles.size());

      // Temperature
      double temp = (2.0/3.0) * total_ke / world.md_particles.size();

      std::cout << "t=" << world.time
                << "  RMS_spread=" << rms_dist
                << "  T=" << temp
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/gas/\n", frame);
  return 0;
}
