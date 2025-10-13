#include "simcore/world.hpp"
#include "modules/md/md_system.hpp"
#include "simcore/base/log.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace minerva;

void setup_particles(World& world, int n_side) {
  const double spacing = 1.2;
  for (int i=0; i<n_side; ++i)
    for (int j=0; j<n_side; ++j)
      for (int k=0; k<n_side; ++k){
        Particle p;
        p.mass = 1.0;
        p.position = Vec3(i*spacing, j*spacing, k*spacing);
        world.md_particles.push(p);
      }
}

double benchmark_md(bool use_nlist, int n_particles_per_side, int steps) {
  World world;
  setup_particles(world, n_particles_per_side);

  MDConfig md_cfg;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.nvt = false;
  md_cfg.use_neighbor_list = use_nlist;
  md_cfg.nlist_skin = 0.3;
  md_cfg.nlist_check_interval = 10;

  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), 1);

  const double dt = 1.0/120.0;

  // Warm-up
  for (int s=0; s<10; ++s) {
    world.step(dt);
  }

  // Benchmark
  auto start = std::chrono::high_resolution_clock::now();
  for (int s=0; s<steps; ++s) {
    world.step(dt);
  }
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed = end - start;
  return elapsed.count();
}

int main() {
  std::cout << "Minerva MD Neighbor List Benchmark\n";
  std::cout << "===================================\n\n";

  const int steps = 500;

  // Test different particle counts
  std::vector<int> sizes = {4, 6, 8, 10};

  std::cout << std::setw(12) << "Particles"
            << std::setw(15) << "All-Pairs (s)"
            << std::setw(15) << "Neighbor List"
            << std::setw(12) << "Speedup\n";
  std::cout << std::string(54, '-') << "\n";

  for (int n_side : sizes) {
    const int n_particles = n_side * n_side * n_side;

    std::cout << std::setw(12) << n_particles << std::flush;

    double time_no_nlist = benchmark_md(false, n_side, steps);
    std::cout << std::setw(15) << std::fixed << std::setprecision(3) << time_no_nlist << std::flush;

    double time_with_nlist = benchmark_md(true, n_side, steps);
    std::cout << std::setw(15) << std::fixed << std::setprecision(3) << time_with_nlist;

    double speedup = time_no_nlist / time_with_nlist;
    std::cout << std::setw(11) << std::fixed << std::setprecision(2) << speedup << "x\n";
  }

  std::cout << "\n";
  return 0;
}
