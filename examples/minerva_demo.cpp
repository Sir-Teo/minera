#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "modules/md/md_system.hpp"
#include "simcore/base/log.hpp"
#include <random>
#include <iostream>

using namespace minerva;

int main(){
  World world;

  // --- Rigid bodies: spawn a small stack of spheres
  {
    std::mt19937 rng(1337);
    std::uniform_real_distribution<double> ux(-1.0, 1.0);

    const int layers = 5;
    for (int y=0; y<layers; ++y){
      for (int x=0; x<layers; ++x){
        for (int z=0; z<layers; ++z){
          RigidBody rb;
          rb.radius = 0.25;
          rb.mass   = 1.0;
          rb.position = Vec3( -0.5 + x*0.55 + 0.05*ux(rng),
                               2.0 + y*0.55,
                              -0.5 + z*0.55 + 0.05*ux(rng) );
          world.rigid_bodies.push_back(rb);
        }
      }
    }
    MINERVA_LOG("Spawned %zu rigid spheres\n", world.rigid_bodies.size());
  }

  // --- MD particles: small Lennard-Jones cloud
  {
    const int n_side = 6;
    const double spacing = 1.2; // loose to avoid huge forces at start
    for (int i=0; i<n_side; ++i)
      for (int j=0; j<n_side; ++j)
        for (int k=0; k<n_side; ++k){
          Particle p;
          p.mass = 1.0;
          p.position = Vec3(3.0 + i*spacing, 1.0 + j*spacing, -2.0 + k*spacing);
          world.md_particles.push(p);
        }
    MINERVA_LOG("Spawned %zu MD particles\n", world.md_particles.size());
  }

  // --- Systems
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.5;
  rb_cfg.ground_y = 0.0;

  MDConfig md_cfg;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.nvt = false; // try NVE first

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), /*substeps*/1);
  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), /*substeps*/1);

  // --- Simulate
  const double dt = 1.0/120.0;  // small time step
  const int steps = 1200;

  for (int s=0; s<steps; ++s){
    world.step(dt);

    if (s % 120 == 0){
      // Print one RB and one MD particle as a quick sanity readout
      const auto& rb = world.rigid_bodies.front();
      const auto& mp = world.md_particles.data.front();
      std::cout << "t=" << world.time
                << "  RB.y=" << rb.position.y
                << "  MD.v2=" << mp.velocity.norm2()
                << "\n";
    }
  }

  MINERVA_LOG("Done. Final time: %.3f s\n", world.time);
  return 0;
}
