#pragma once
#include "simcore/scheduler/scheduler.hpp"

namespace minerva {

struct RigidBodySystemConfig {
  double restitution{0.5};  // bounce
  double friction{0.3};     // not used yet
  double ground_y{0.0};     // y = ground plane
};

class RigidBodySystem final : public ISystem {
public:
  explicit RigidBodySystem(const RigidBodySystemConfig& cfg) : cfg_(cfg) {}
  const char* name() const override { return "RigidBodySystem"; }
  void step(World& world, double dt) override;

private:
  RigidBodySystemConfig cfg_;
};

} // namespace minerva
