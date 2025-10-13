#pragma once
#include "simcore/scheduler/scheduler.hpp"

namespace minerva {

struct RigidBodySystemConfig {
  double restitution{0.5};  // bounce
  double friction{0.3};     // not used yet
  double ground_y{0.0};     // y = ground plane
  int    substeps{4};       // internal RB substeps per world step (increased from 2)
  int    pair_iterations{32}; // iterations of pair resolution per substep (increased from 16)
  double penetration_slop{1e-5}; // acceptable penetration before early-out (tighter)
  double contact_offset{1e-3};   // target extra separation to avoid re-penetration (larger buffer)
  double baumgarte{0.8};         // fraction of correction to apply per iteration (more aggressive)
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
