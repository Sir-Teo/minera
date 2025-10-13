#include "modules/rb/rigid_body_system.hpp"
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <functional>

namespace minerva {

void RigidBodySystem::step(World& world, double dt){
  const int substeps = std::max(1, cfg_.substeps);
  const double h = dt / static_cast<double>(substeps);

  struct CellKey {
    int x, y, z;
    bool operator==(const CellKey& o) const { return x==o.x && y==o.y && z==o.z; }
  };
  struct CellKeyHash {
    std::size_t operator()(const CellKey& k) const noexcept {
      std::size_t h = 1469598103934665603ull;
      auto mix = [&](int v){
        h ^= static_cast<std::size_t>(v) + 0x9e3779b97f4a7c15ull + (h<<6) + (h>>2);
      };
      mix(k.x); mix(k.y); mix(k.z);
      return h;
    }
  };

  for (int sub = 0; sub < substeps; ++sub){
    // 1) Integrate with gravity and handle ground plane
    for (auto& rb : world.rigid_bodies){
      if (rb.kinematic || rb.mass <= 0.0) continue;
      rb.velocity += world.gravity * h;
      rb.position += rb.velocity * h;
      const double target_y = cfg_.ground_y + rb.radius + cfg_.contact_offset;
      if (rb.position.y < target_y){
        rb.position.y = target_y;
        double vn = rb.velocity.y;
        if (vn < 0.0){
          rb.velocity.y = -cfg_.restitution * vn;
          rb.velocity.x *= 0.98;
          rb.velocity.z *= 0.98;
        }
      }
    }

    // 2) Broad-phase cell size from maximum radius
    double max_r = 0.0;
    for (const auto& rb : world.rigid_bodies){
      if (rb.radius > max_r) max_r = rb.radius;
    }
    if (max_r > 0.0){
      const double cell_size = std::max(2.0 * max_r, 1e-6);
      auto cell_of = [&](const Vec3& p){
        return CellKey{ static_cast<int>(std::floor(p.x / cell_size)),
                        static_cast<int>(std::floor(p.y / cell_size)),
                        static_cast<int>(std::floor(p.z / cell_size)) };
      };

      // 3) Iterative pair resolution
      const int max_iters = std::max(1, cfg_.pair_iterations);
      const double tol = std::max(0.0, cfg_.penetration_slop);
      for (int iter = 0; iter < max_iters; ++iter){
        std::unordered_map<CellKey, std::vector<int>, CellKeyHash> grid;
        grid.reserve(world.rigid_bodies.size()*2);
        for (int i = 0; i < static_cast<int>(world.rigid_bodies.size()); ++i){
          const CellKey key = cell_of(world.rigid_bodies[i].position);
          grid[key].push_back(i);
        }

        double max_pen = 0.0;
        const int neighbor_range = 1;
        for (int i = 0; i < static_cast<int>(world.rigid_bodies.size()); ++i){
          auto& a = world.rigid_bodies[i];
          const CellKey ka = cell_of(a.position);
          for (int dx = -neighbor_range; dx <= neighbor_range; ++dx){
            for (int dy = -neighbor_range; dy <= neighbor_range; ++dy){
              for (int dz = -neighbor_range; dz <= neighbor_range; ++dz){
                const CellKey kb{ka.x + dx, ka.y + dy, ka.z + dz};
                auto it = grid.find(kb);
                if (it == grid.end()) continue;
                const auto& indices = it->second;
                for (int j : indices){
                  if (j <= i) continue;
                  auto& b = world.rigid_bodies[j];
                  const double min_dist = a.radius + b.radius;
                  const Vec3 d = b.position - a.position;
                  const double d2 = d.norm2();
                  const double target_sep = min_dist + cfg_.contact_offset;
                  if (d2 >= target_sep*target_sep) continue;
                  const double dist = std::sqrt(std::max(d2, 1e-16));
                  Vec3 n = (dist > 1e-12) ? (d / dist) : Vec3::unit_x();
                  
                  const double penetration = std::max(target_sep - dist, 0.0);
                  if (penetration > max_pen) max_pen = penetration;
                  auto grounded = [&](const auto& rb){
                    return (rb.position.y - rb.radius) <= (cfg_.ground_y + cfg_.contact_offset + 1e-6);
                  };
                  const bool a_ground = grounded(a);
                  const bool b_ground = grounded(b);
                  // a gets moved by -corr ~ -n; this pushes a downward if n.y > 0
                  // b gets moved by +corr ~ +n; this pushes b downward if n.y < 0
                  const double ny = n.y;
                  const bool a_push_down = ny > 0.2;
                  const bool b_push_down = ny < -0.2;
                  const bool a_static = a.kinematic || a.mass <= 0.0 || (a_ground && a_push_down);
                  const bool b_static = b.kinematic || b.mass <= 0.0 || (b_ground && b_push_down);
                  const double invMa = a_static ? 0.0 : 1.0 / a.mass;
                  const double invMb = b_static ? 0.0 : 1.0 / b.mass;
                  const double inv_sum = invMa + invMb;
                  if (inv_sum <= 0.0) continue;
                  const Vec3 corr = n * ((cfg_.baumgarte * penetration) / inv_sum);
                  if (!a_static) a.position -= corr * invMa;
                  if (!b_static) b.position += corr * invMb;
                  const Vec3 relv = b.velocity - a.velocity;
                  const double vn = relv.dot(n);
                  if (vn < 0.0){
                    const double jimp = -(1.0 + cfg_.restitution) * vn / inv_sum;
                    const Vec3 impulse = n * jimp;
                    if (!a_static) a.velocity -= impulse * invMa;
                    if (!b_static) b.velocity += impulse * invMb;
                    a.velocity *= 0.999;
                    b.velocity *= 0.999;
                  }
                }
              }
            }
          }
        }
        if (max_pen < tol) break;
      }
    }

    // 4) Final ground-plane clamp
    for (auto& rb : world.rigid_bodies){
      const double target_y2 = cfg_.ground_y + rb.radius + cfg_.contact_offset;
      if (rb.position.y < target_y2){
        rb.position.y = target_y2;
        if (rb.velocity.y < 0.0){
          rb.velocity.y = -cfg_.restitution * rb.velocity.y;
          rb.velocity.x *= 0.98;
          rb.velocity.z *= 0.98;
        }
      }
    }
  }
}

} // namespace minerva
