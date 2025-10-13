#pragma once
#include "simcore/scheduler/scheduler.hpp"
#include "simcore/spatial/neighbor_list.hpp"
#include "simcore/math/vec3.hpp"
#include <memory>
#include <vector>

namespace minerva {

struct ParticleSet;  // forward declaration

struct MDConfig {
  double epsilon{1.0};
  double sigma{1.0};
  double rcut_sigma{2.5};  // cutoff in units of sigma
  bool   nvt{false};
  double temp{1.0};
  double tau_thermo{1.0};  // Berendsen time constant

  // Neighbor list settings
  bool   use_neighbor_list{true};
  double nlist_skin{0.3};  // Verlet skin distance
  int    nlist_check_interval{10};  // Check for rebuild every N steps
};

class MDSystem final : public ISystem {
public:
  explicit MDSystem(const MDConfig& cfg);
  const char* name() const override { return "MDSystem"; }
  void step(World& world, double dt) override;

  // Access neighbor list stats
  const NeighborListStats& neighbor_stats() const;

private:
  MDConfig cfg_;
  std::unique_ptr<NeighborList> neighbor_list_;
  int steps_since_check_{0};

  // simple velocity-verlet integrator
  void integrate(World& world, double dt);

  // Helper to extract positions from ParticleSet
  void extract_positions(const ParticleSet& ps, std::vector<Vec3>& positions);
};

} // namespace minerva
