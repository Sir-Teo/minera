#include "simcore/spatial/neighbor_list.hpp"
#include "simcore/base/log.hpp"
#include <algorithm>
#include <cmath>

namespace minerva {

NeighborList::NeighborList(const NeighborListConfig& cfg) : cfg_(cfg) {
  setup_grid();
}

void NeighborList::setup_grid() {
  // Determine cell size: should be at least cutoff + skin
  const double min_cell_size = (cfg_.cutoff + cfg_.skin) * cfg_.cell_size_factor;

  // Calculate grid dimensions
  Vec3 domain_size = cfg_.domain_max - cfg_.domain_min;

  nx_ = std::max(1, static_cast<int>(domain_size.x / min_cell_size));
  ny_ = std::max(1, static_cast<int>(domain_size.y / min_cell_size));
  nz_ = std::max(1, static_cast<int>(domain_size.z / min_cell_size));

  // Actual cell size
  cell_size_.x = domain_size.x / static_cast<double>(nx_);
  cell_size_.y = domain_size.y / static_cast<double>(ny_);
  cell_size_.z = domain_size.z / static_cast<double>(nz_);

  // Allocate cell grid
  const int total_cells = nx_ * ny_ * nz_;
  cells_.resize(static_cast<std::size_t>(total_cells));

  MINERVA_LOG("NeighborList: grid %dx%dx%d (%d cells), cell_size=(%.3f,%.3f,%.3f)\n",
              nx_, ny_, nz_, total_cells, cell_size_.x, cell_size_.y, cell_size_.z);
}

void NeighborList::get_cell_coords(const Vec3& pos, int& ix, int& iy, int& iz) const {
  // Clamp position to domain and compute cell coordinates
  Vec3 rel_pos = pos - cfg_.domain_min;

  ix = static_cast<int>(rel_pos.x / cell_size_.x);
  iy = static_cast<int>(rel_pos.y / cell_size_.y);
  iz = static_cast<int>(rel_pos.z / cell_size_.z);

  // Clamp to valid range
  ix = std::max(0, std::min(nx_ - 1, ix));
  iy = std::max(0, std::min(ny_ - 1, iy));
  iz = std::max(0, std::min(nz_ - 1, iz));
}

int NeighborList::get_cell_index(const Vec3& pos) const {
  int ix, iy, iz;
  get_cell_coords(pos, ix, iy, iz);
  return ix + nx_ * (iy + ny_ * iz);
}

void NeighborList::build(const std::vector<Vec3>& positions) {
  const std::size_t n = positions.size();

  // Clear previous data
  pairs_.clear();
  for (auto& cell : cells_) {
    cell.clear();
  }

  // Assign particles to cells
  for (std::size_t i = 0; i < n; ++i) {
    const int cell_idx = get_cell_index(positions[i]);
    cells_[static_cast<std::size_t>(cell_idx)].push_back(i);
  }

  // Build pair list
  build_pairs(positions);

  // Store reference positions for displacement tracking
  ref_positions_ = positions;
  valid_ = true;

  // Update stats
  if (cfg_.enable_stats) {
    stats_.total_builds++;
    stats_.num_pairs = pairs_.size();
  }

  MINERVA_LOG("NeighborList: rebuilt with %zu pairs for %zu particles\n", pairs_.size(), n);
}

void NeighborList::build_pairs(const std::vector<Vec3>& positions) {
  const double r_list_sq = (cfg_.cutoff + cfg_.skin) * (cfg_.cutoff + cfg_.skin);

  // Iterate over all cells
  for (int iz = 0; iz < nz_; ++iz) {
    for (int iy = 0; iy < ny_; ++iy) {
      for (int ix = 0; ix < nx_; ++ix) {
        const int cell_idx = ix + nx_ * (iy + ny_ * iz);
        const auto& cell_particles = cells_[static_cast<std::size_t>(cell_idx)];

        // Self-interactions within cell
        for (std::size_t a = 0; a < cell_particles.size(); ++a) {
          const std::size_t i = cell_particles[a];
          for (std::size_t b = a + 1; b < cell_particles.size(); ++b) {
            const std::size_t j = cell_particles[b];

            const Vec3 rij = positions[j] - positions[i];
            const double r2 = rij.norm2();

            if (r2 < r_list_sq) {
              pairs_.push_back({i, j});
            }
          }
        }

        // Interactions with neighboring cells (half-shell to avoid double counting)
        for (int dz = 0; dz <= 1; ++dz) {
          for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
              // Skip self-cell (already done above)
              if (dz == 0 && dy == 0 && dx == 0) continue;

              // Skip lower half-shell when dz == 0
              if (dz == 0 && (dy < 0 || (dy == 0 && dx < 0))) continue;

              const int nx = ix + dx;
              const int ny = iy + dy;
              const int nz = iz + dz;

              // Check bounds
              if (nx < 0 || nx >= nx_ || ny < 0 || ny >= ny_ || nz < 0 || nz >= nz_) {
                continue;
              }

              const int neighbor_idx = nx + nx_ * (ny + ny_ * nz);
              const auto& neighbor_particles = cells_[static_cast<std::size_t>(neighbor_idx)];

              // Check all pairs between current cell and neighbor cell
              for (std::size_t i : cell_particles) {
                for (std::size_t j : neighbor_particles) {
                  const Vec3 rij = positions[j] - positions[i];
                  const double r2 = rij.norm2();

                  if (r2 < r_list_sq) {
                    pairs_.push_back({i, j});
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

bool NeighborList::needs_rebuild(const std::vector<Vec3>& positions) const {
  if (!valid_) return true;
  if (ref_positions_.size() != positions.size()) return true;

  // Check maximum displacement
  double max_disp_sq = 0.0;
  for (std::size_t i = 0; i < positions.size(); ++i) {
    const Vec3 disp = positions[i] - ref_positions_[i];
    const double disp_sq = disp.norm2();
    max_disp_sq = std::max(max_disp_sq, disp_sq);
  }

  // Rebuild if any particle moved more than skin/2
  // (This ensures particles can't move out of neighbor list range)
  const double rebuild_threshold = (cfg_.skin * 0.5) * (cfg_.skin * 0.5);

  if (cfg_.enable_stats) {
    const_cast<NeighborListStats&>(stats_).total_checks++;
    const_cast<NeighborListStats&>(stats_).max_displacement = std::sqrt(max_disp_sq);
  }

  return max_disp_sq > rebuild_threshold;
}

} // namespace minerva
