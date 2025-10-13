#pragma once
#include "simcore/state/rigid_body.hpp"
#include "simcore/state/particle.hpp"
#include "simcore/math/vec3.hpp"
#include "simcore/scheduler/scheduler.hpp"
#include <vector>
#include <cstdint>

namespace minerva {

struct World {
  // Global settings
  double time{0.0};
  Vec3 gravity{0.0, -9.81, 0.0};

  // State containers
  std::vector<RigidBody> rigid_bodies;
  ParticleSet md_particles;

  // Orchestration
  Scheduler scheduler;

  void step(double dt){
    scheduler.tick(*this, dt);
    time += dt;
  }
};

} // namespace minerva
