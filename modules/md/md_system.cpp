#include "modules/md/md_system.hpp"
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <vector>
#include <cmath>

namespace minerva {

static inline void lj_forces(const MDConfig& cfg,
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
      // -> factorized as 24*epsilon*inv_r2*(2*(sigma^12)*inv_r6^2 - (sigma^6)*inv_r6)
      const double mag = 24.0 * cfg.epsilon * inv_r2 * ( 2.0*sig12*inv_r6*inv_r6 - sig6*inv_r6 );
      const Vec3 fij = (mag) * rij; // since rij not normalized, mag already has inv_r factor

      forces[i] -= fij;
      forces[j] += fij;
    }
  }
}

void MDSystem::integrate(World& world, double dt)
{
  auto& ps = world.md_particles;
  const std::size_t n = ps.size();

  // 1) compute forces at t
  static thread_local std::vector<Vec3> forces;
  forces.resize(n);
  lj_forces(cfg_, ps, forces);

  // 2) velocity half-step, position full-step
  for (std::size_t i=0; i<n; ++i){
    auto& p = ps.data[i];
    const Vec3 a = (1.0 / p.mass) * forces[i];
    p.velocity += 0.5 * dt * a;
    p.position += dt * p.velocity;
  }

  // 3) forces at t+dt
  lj_forces(cfg_, ps, forces);

  // 4) velocity half-step
  for (std::size_t i=0; i<n; ++i){
    auto& p = ps.data[i];
    const Vec3 a = (1.0 / p.mass) * forces[i];
    p.velocity += 0.5 * dt * a;
  }

  // 5) optional simple Berendsen thermostat
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

} // namespace minerva
