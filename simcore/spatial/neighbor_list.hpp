#pragma once
#include "simcore/math/vec3.hpp"
#include <vector>
#include <cstddef>

namespace minerva {

// Pair of particle indices for neighbor interactions
struct NeighborPair {
  std::size_t i;
  std::size_t j;
};

// Configuration for neighbor list construction
struct NeighborListConfig {
  double cutoff{2.5};        // Interaction cutoff distance
  double skin{0.3};          // Extra distance for Verlet list (reduces rebuilds)
  double cell_size_factor{1.0}; // Cell size = (cutoff + skin) * factor

  // Domain bounds (for cell partitioning)
  Vec3 domain_min{-10, -10, -10};
  Vec3 domain_max{10, 10, 10};

  bool enable_stats{false};  // Track rebuild statistics
};

// Statistics for neighbor list performance
struct NeighborListStats {
  std::size_t total_builds{0};
  std::size_t total_checks{0};
  double max_displacement{0.0};
  std::size_t num_pairs{0};

  void reset() {
    total_builds = 0;
    total_checks = 0;
    max_displacement = 0.0;
    num_pairs = 0;
  }
};

// Cell-list based neighbor list with Verlet skin
class NeighborList {
public:
  explicit NeighborList(const NeighborListConfig& cfg);

  // Build the neighbor list from particle positions
  void build(const std::vector<Vec3>& positions);

  // Check if rebuild is needed based on particle displacement
  bool needs_rebuild(const std::vector<Vec3>& positions) const;

  // Access the neighbor pairs
  const std::vector<NeighborPair>& pairs() const { return pairs_; }

  // Get statistics
  const NeighborListStats& stats() const { return stats_; }

  // Force a rebuild on next check
  void invalidate() { valid_ = false; }

private:
  NeighborListConfig cfg_;
  NeighborListStats stats_;

  // Neighbor pair list
  std::vector<NeighborPair> pairs_;

  // Reference positions for displacement tracking
  std::vector<Vec3> ref_positions_;

  // Cell list data
  Vec3 cell_size_;
  int nx_, ny_, nz_;  // Grid dimensions
  std::vector<std::vector<std::size_t>> cells_;  // Particle indices per cell

  bool valid_{false};

  // Helper methods
  void setup_grid();
  int get_cell_index(const Vec3& pos) const;
  void get_cell_coords(const Vec3& pos, int& ix, int& iy, int& iz) const;
  void build_pairs(const std::vector<Vec3>& positions);
};

} // namespace minerva
