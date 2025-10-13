#include "modules/md/md_system.hpp"
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace minerva {

// Force calculation using ALL pairs (O(N²) - for comparison or fallback)
static inline void lj_forces_all_pairs(const MDConfig& cfg,
                                        const ParticleSet& ps,
                                        std::vector<Vec3>& forces)
{
  const double rc = cfg.rcut_sigma * cfg.sigma;
  const double rc2 = rc * rc;
  const double sig2 = cfg.sigma * cfg.sigma;

  const std::size_t n = ps.size();
  std::fill(forces.begin(), forces.end(), Vec3::zero());

  for (std::size_t i=0; i<n; ++i){
    const Vec3 pi = ps.data[i].position;
    for (std::size_t j=i+1; j<n; ++j){
      Vec3 rij = ps.data[j].position - pi;
      const double r2 = rij.norm2();
      if (r2 > rc2 || r2 == 0.0) continue;

      const double inv_r2 = 1.0 / r2;
      const double inv_r6 = inv_r2 * inv_r2 * inv_r2;
      const double sig6 = sig2*sig2*sig2;
      const double sig12 = sig6*sig6;

      // |F| = 24*epsilon*(2*(sigma^12)/r^13 - (sigma^6)/r^7)
      const double mag = 24.0 * cfg.epsilon * inv_r2 * ( 2.0*sig12*inv_r6*inv_r6 - sig6*inv_r6 );
      const Vec3 fij = (mag) * rij;

      forces[i] -= fij;
      forces[j] += fij;
    }
  }
}

// Force calculation using neighbor list (O(N) with good spatial locality)
static inline void lj_forces_neighbor_list(const MDConfig& cfg,
                                            const ParticleSet& ps,
                                            const std::vector<NeighborPair>& pairs,
                                            std::vector<Vec3>& forces)
{
  const double rc = cfg.rcut_sigma * cfg.sigma;
  const double rc2 = rc * rc;
  const double sig2 = cfg.sigma * cfg.sigma;

  std::fill(forces.begin(), forces.end(), Vec3::zero());

  // Iterate only over neighbor pairs
  for (const auto& pair : pairs) {
    const std::size_t i = pair.i;
    const std::size_t j = pair.j;

    Vec3 rij = ps.data[j].position - ps.data[i].position;
    const double r2 = rij.norm2();

    // Note: pairs include all within cutoff+skin, so we still need cutoff check
    if (r2 > rc2 || r2 == 0.0) continue;

    const double inv_r2 = 1.0 / r2;
    const double inv_r6 = inv_r2 * inv_r2 * inv_r2;
    const double sig6 = sig2*sig2*sig2;
    const double sig12 = sig6*sig6;

    const double mag = 24.0 * cfg.epsilon * inv_r2 * ( 2.0*sig12*inv_r6*inv_r6 - sig6*inv_r6 );
    const Vec3 fij = (mag) * rij;

    forces[i] -= fij;
    forces[j] += fij;
  }
}

MDSystem::MDSystem(const MDConfig& cfg) : cfg_(cfg) {
  if (cfg_.use_neighbor_list) {
    // Setup neighbor list with appropriate parameters
    NeighborListConfig nl_cfg;
    nl_cfg.cutoff = cfg_.rcut_sigma * cfg_.sigma;
    nl_cfg.skin = cfg_.nlist_skin;
    nl_cfg.enable_stats = true;

    // Estimate domain bounds (will be updated on first build)
    // For now use reasonable defaults - will auto-expand if needed
    nl_cfg.domain_min = Vec3(-10, -10, -10);
    nl_cfg.domain_max = Vec3(10, 10, 10);

    neighbor_list_ = std::make_unique<NeighborList>(nl_cfg);
    MINERVA_LOG("MDSystem: neighbor list enabled (cutoff=%.3f, skin=%.3f)\n",
                nl_cfg.cutoff, nl_cfg.skin);
  } else {
    MINERVA_LOG("MDSystem: using all-pairs force calculation (O(N²))\n");
  }
}

void MDSystem::extract_positions(const ParticleSet& ps, std::vector<Vec3>& positions) {
  positions.resize(ps.size());
  for (std::size_t i = 0; i < ps.size(); ++i) {
    positions[i] = ps.data[i].position;
  }
}

void MDSystem::integrate(World& world, double dt)
{
  auto& ps = world.md_particles;
  const std::size_t n = ps.size();

  static thread_local std::vector<Vec3> forces;
  static thread_local std::vector<Vec3> positions;
  forces.resize(n);

  // 1) Update neighbor list if needed
  if (cfg_.use_neighbor_list && neighbor_list_) {
    steps_since_check_++;

    // Check for rebuild periodically
    if (steps_since_check_ >= cfg_.nlist_check_interval) {
      extract_positions(ps, positions);

      if (neighbor_list_->needs_rebuild(positions)) {
        // Update domain bounds based on current positions
        Vec3 pmin = positions[0], pmax = positions[0];
        for (const auto& p : positions) {
          pmin.x = std::min(pmin.x, p.x);
          pmin.y = std::min(pmin.y, p.y);
          pmin.z = std::min(pmin.z, p.z);
          pmax.x = std::max(pmax.x, p.x);
          pmax.y = std::max(pmax.y, p.y);
          pmax.z = std::max(pmax.z, p.z);
        }

        // Add margin to domain
        const double margin = 2.0 * (cfg_.rcut_sigma * cfg_.sigma + cfg_.nlist_skin);
        pmin = pmin - Vec3(margin, margin, margin);
        pmax = pmax + Vec3(margin, margin, margin);

        // Create new neighbor list with updated bounds
        NeighborListConfig nl_cfg;
        nl_cfg.cutoff = cfg_.rcut_sigma * cfg_.sigma;
        nl_cfg.skin = cfg_.nlist_skin;
        nl_cfg.domain_min = pmin;
        nl_cfg.domain_max = pmax;
        nl_cfg.enable_stats = true;

        neighbor_list_ = std::make_unique<NeighborList>(nl_cfg);
        neighbor_list_->build(positions);
      }
      steps_since_check_ = 0;
    }

    // Ensure neighbor list exists (first step)
    if (neighbor_list_->pairs().empty() && n > 0) {
      extract_positions(ps, positions);
      neighbor_list_->build(positions);
    }
  }

  // 2) Compute forces at t
  if (cfg_.use_neighbor_list && neighbor_list_ && !neighbor_list_->pairs().empty()) {
    lj_forces_neighbor_list(cfg_, ps, neighbor_list_->pairs(), forces);
  } else {
    lj_forces_all_pairs(cfg_, ps, forces);
  }

  // 3) Velocity half-step, position full-step
  for (std::size_t i=0; i<n; ++i){
    auto& p = ps.data[i];
    const Vec3 a = (1.0 / p.mass) * forces[i];
    p.velocity += 0.5 * dt * a;
    p.position += dt * p.velocity;
  }

  // 4) Forces at t+dt
  if (cfg_.use_neighbor_list && neighbor_list_ && !neighbor_list_->pairs().empty()) {
    lj_forces_neighbor_list(cfg_, ps, neighbor_list_->pairs(), forces);
  } else {
    lj_forces_all_pairs(cfg_, ps, forces);
  }

  // 5) Velocity half-step
  for (std::size_t i=0; i<n; ++i){
    auto& p = ps.data[i];
    const Vec3 a = (1.0 / p.mass) * forces[i];
    p.velocity += 0.5 * dt * a;
  }

  // 6) Optional simple Berendsen thermostat
  if (cfg_.nvt){
    double ke = 0.0;
    for (std::size_t i=0; i<n; ++i){
      ke += 0.5 * ps.data[i].mass * ps.data[i].velocity.norm2();
    }
    const double Tinst = (2.0/3.0) * (ke / static_cast<double>(n)); // k_B = 1
    const double lambda = std::sqrt(1.0 + (dt / cfg_.tau_thermo) * ((cfg_.temp / Tinst) - 1.0));
    for (std::size_t i=0; i<n; ++i){
      ps.data[i].velocity *= lambda;
    }
  }
}

void MDSystem::step(World& world, double dt){
  integrate(world, dt);
}

const NeighborListStats& MDSystem::neighbor_stats() const {
  static NeighborListStats empty_stats;
  if (neighbor_list_) {
    return neighbor_list_->stats();
  }
  return empty_stats;
}

} // namespace minerva
