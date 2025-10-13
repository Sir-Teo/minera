#include "modules/rb/rigid_body_system.hpp"
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <algorithm>

namespace minerva {

void RigidBodySystem::step(World& world, double dt){
  // Integrate + ground-plane collisions (very simple)
  for (auto& rb : world.rigid_bodies){
    if (rb.kinematic || rb.mass <= 0.0) continue;

    // Semi-implicit Euler
    rb.velocity += world.gravity * dt;
    rb.position += rb.velocity * dt;

    // Ground plane collision at y = ground_y (penalty-free impulse style)
    const double bottom = rb.position.y - rb.radius;
    if (bottom < cfg_.ground_y){
      // Correct position
      rb.position.y = cfg_.ground_y + rb.radius;

      // Reflect velocity with restitution on normal component (n = +Y)
      double vn = rb.velocity.y;     // normal component along +Y
      if (vn < 0.0){
        rb.velocity.y = -cfg_.restitution * vn;
        // crude tangential damping to mimic frictional losses
        rb.velocity.x *= 0.98;
        rb.velocity.z *= 0.98;
      }
    }
  }
}

} // namespace minerva
