#pragma once
#include "simcore/scheduler/scheduler.hpp"

namespace minerva {

struct RigidBodySystemConfig {
  double restitution{0.5};  // bounce
  double friction{0.3};     // not used yet
  double ground_y{0.0};     // y = ground plane
  int    substeps{2};       // internal RB substeps per world step
  int    pair_iterations{16}; // iterations of pair resolution per substep
  double penetration_slop{1e-4}; // acceptable penetration before early-out
  double contact_offset{1e-4};   // target extra separation to avoid re-penetration
  double baumgarte{1.0};         // fraction of correction to apply per iteration
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
