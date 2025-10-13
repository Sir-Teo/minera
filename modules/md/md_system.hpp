#pragma once
#include "simcore/scheduler/scheduler.hpp"

namespace minerva {

struct MDConfig {
  double epsilon{1.0};
  double sigma{1.0};
  double rcut_sigma{2.5};  // cutoff in units of sigma
  bool   nvt{false};
  double temp{1.0};
  double tau_thermo{1.0};  // Berendsen time constant
};

class MDSystem final : public ISystem {
public:
  explicit MDSystem(const MDConfig& cfg) : cfg_(cfg) {}
  const char* name() const override { return "MDSystem"; }
  void step(World& world, double dt) override;

private:
  MDConfig cfg_;
  // simple velocity-verlet integrator
  void integrate(World& world, double dt);
};

} // namespace minerva
