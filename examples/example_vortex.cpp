#include "simcore/world.hpp"
#include "modules/md/md_system.hpp"
#include "simcore/io/csv_writer.hpp"
#include "simcore/io/vtk_writer.hpp"
#include "simcore/base/log.hpp"
#include <iostream>
#include <memory>
#include <random>
#include <cmath>

using namespace minerva;

/**
 * Example: Vortex
 *
 * MD particles initialized with circular velocity pattern creating a vortex.
 * Demonstrates rotational flow and angular momentum conservation.
 */
int main(){
  World world;
  world.gravity = Vec3(0, 0, 0); // No gravity for clean vortex

  std::mt19937 rng(123);
  std::uniform_real_distribution<double> radius_dist(1.0, 5.0);
  std::uniform_real_distribution<double> height_dist(-2.0, 2.0);
  std::uniform_real_distribution<double> angle_dist(0, 2.0 * M_PI);

  // Create particles in a cylindrical vortex pattern
  const int n_particles = 800;

  for (int i = 0; i < n_particles; ++i){
    Particle p;
    p.mass = 1.0;

    // Random cylindrical coordinates
    double r = radius_dist(rng);
    double theta = angle_dist(rng);
    double y = height_dist(rng);

    // Position in cylinder
    p.position = Vec3(
      r * std::cos(theta),
      y,
      r * std::sin(theta)
    );

    // Tangential velocity (creates rotation)
    // Velocity decreases with radius (like a vortex)
    double v_tangent = 8.0 / (r + 1.0);
    p.velocity = Vec3(
      -v_tangent * std::sin(theta),
      0.0,
      v_tangent * std::cos(theta)
    );

    world.md_particles.push(p);
  }

  MINERVA_LOG("Vortex: %zu particles in rotating pattern\n", world.md_particles.size());

  // MD configuration
  MDConfig md_cfg;
  md_cfg.epsilon = 0.3; // Weak interactions to preserve vortex
  md_cfg.sigma = 1.0;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.use_neighbor_list = true;
  md_cfg.nvt = false; // NVE to conserve angular momentum
  md_cfg.nlist_skin = 0.3;

  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), 1);

  // I/O setup
  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/vortex";
  csv_cfg.prefix = "vortex";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  VTKWriterConfig vtk_cfg;
  vtk_cfg.output_dir = "output/vortex";
  vtk_cfg.prefix = "vortex";
  auto vtk_writer = std::make_unique<VTKWriter>(vtk_cfg);

  // Simulate
  const double dt = 1.0/150.0;
  const int steps = 3000; // 20 seconds
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

    if (s % 150 == 0){
      // Calculate angular momentum
      Vec3 L(0, 0, 0);
      for (const auto& p : world.md_particles.data){
        Vec3 r = p.position;
        Vec3 v = p.velocity * p.mass;
        // L = r × mv
        L.x += r.y * v.z - r.z * v.y;
        L.y += r.z * v.x - r.x * v.z;
        L.z += r.x * v.y - r.y * v.x;
      }

      std::cout << "t=" << world.time
                << "  Lz=" << L.z
                << "\n";
    }
  }

  vtk_writer->finalize();
  MINERVA_LOG("Done. Output: %d frames in output/vortex/\n", frame);
  return 0;
}
