#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "simcore/utils/overlap_checker.hpp"
#include "simcore/io/csv_writer.hpp"
#include <iostream>

using namespace minerva;

int main(){
  World world;
  world.gravity = Vec3(0, -9.81, 0);

  // Create just 10 spheres in a line - simple test case
  const double radius = 0.2;
  const double spacing = 0.5; // Should be OK: diameter=0.4, spacing=0.5

  for (int i = 0; i < 10; ++i){
    RigidBody rb;
    rb.radius = radius;
    rb.mass = 1.0;
    rb.position = Vec3(i * spacing, 2.0, 0.0);
    rb.velocity = Vec3(0, 0, 0);
    world.rigid_bodies.push_back(rb);
  }

  std::cout << "=== BEFORE overlap resolution ===" << std::endl;
  int overlaps_before = check_rigid_body_overlaps(world);
  std::cout << "Overlaps found: " << overlaps_before << std::endl;

  std::cout << "\n=== Running overlap resolution ===" << std::endl;
  resolve_initial_overlaps(world, 100);

  std::cout << "\n=== AFTER overlap resolution ===" << std::endl;
  int overlaps_after = check_rigid_body_overlaps(world);
  std::cout << "Overlaps found: " << overlaps_after << std::endl;

  // Check actual distances
  std::cout << "\n=== Checking actual distances ===" << std::endl;
  for (size_t i = 0; i < world.rigid_bodies.size() - 1; ++i){
    const auto& a = world.rigid_bodies[i];
    const auto& b = world.rigid_bodies[i+1];
    Vec3 d = b.position - a.position;
    double dist = std::sqrt(d.norm2());
    double min_dist = a.radius + b.radius;
    double gap = dist - min_dist;
    std::cout << "Sphere " << i << " to " << (i+1)
              << ": dist=" << dist
              << " min_dist=" << min_dist
              << " gap=" << gap;
    if (gap < 0) std::cout << " *** OVERLAP ***";
    std::cout << std::endl;
  }

  // Now run simulation for 10 steps and check again
  RigidBodySystemConfig rb_cfg;
  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), 1);

  CSVWriterConfig csv_cfg;
  csv_cfg.output_dir = "output/test";
  csv_cfg.prefix = "test";
  auto csv_writer = std::make_unique<CSVWriter>(csv_cfg);

  std::cout << "\n=== Running 10 simulation steps ===" << std::endl;
  for (int step = 0; step < 10; ++step){
    world.step(1.0/120.0);

    int overlaps = 0;
    for (size_t i = 0; i < world.rigid_bodies.size(); ++i){
      for (size_t j = i+1; j < world.rigid_bodies.size(); ++j){
        const auto& a = world.rigid_bodies[i];
        const auto& b = world.rigid_bodies[j];
        Vec3 d = b.position - a.position;
        double dist = std::sqrt(d.norm2());
        double min_dist = a.radius + b.radius;
        if (dist < min_dist - 1e-6){
          overlaps++;
        }
      }
    }

    csv_writer->write(world, step);
    std::cout << "Step " << step << ": overlaps = " << overlaps << std::endl;
  }

  return 0;
}
