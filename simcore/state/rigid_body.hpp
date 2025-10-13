#pragma once
#include "simcore/math/vec3.hpp"

namespace minerva {

// Minimal rigid body as a sphere for now
struct RigidBody {
  Vec3 position{0,0,0};
  Vec3 velocity{0,0,0};
  double mass{1.0};
  double radius{0.5};
  bool   kinematic{false}; // if true, we ignore dynamics
};

} // namespace minerva
