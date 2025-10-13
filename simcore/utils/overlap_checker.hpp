#pragma once
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <cmath>

namespace minerva {

/**
 * Check for overlapping rigid bodies and report violations.
 * Returns the number of overlapping pairs found.
 */
inline int check_rigid_body_overlaps(const World& world, double tolerance = 1e-6) {
  int overlap_count = 0;
  double max_overlap = 0.0;

  const auto& bodies = world.rigid_bodies;

  for (size_t i = 0; i < bodies.size(); ++i) {
    for (size_t j = i + 1; j < bodies.size(); ++j) {
      const auto& a = bodies[i];
      const auto& b = bodies[j];

      const Vec3 d = b.position - a.position;
      const double dist = std::sqrt(d.norm2());
      const double min_dist = a.radius + b.radius;
      const double overlap = min_dist - dist;

      if (overlap > tolerance) {
        overlap_count++;
        if (overlap > max_overlap) {
          max_overlap = overlap;
        }
      }
    }
  }

  if (overlap_count > 0) {
    MINERVA_LOG("WARNING: Found %d overlapping sphere pairs!\n", overlap_count);
    MINERVA_LOG("         Maximum overlap: %.6f units\n", max_overlap);
    MINERVA_LOG("         This will cause unrealistic physics behavior.\n");
  }

  return overlap_count;
}

/**
 * Resolve initial overlaps by pushing spheres apart.
 * Uses iterative position-based corrections.
 */
inline void resolve_initial_overlaps(World& world, int max_iterations = 100) {
  MINERVA_LOG("Resolving initial overlaps...\n");

  for (int iter = 0; iter < max_iterations; ++iter) {
    double max_overlap = 0.0;
    int corrections = 0;

    for (size_t i = 0; i < world.rigid_bodies.size(); ++i) {
      for (size_t j = i + 1; j < world.rigid_bodies.size(); ++j) {
        auto& a = world.rigid_bodies[i];
        auto& b = world.rigid_bodies[j];

        const Vec3 d = b.position - a.position;
        const double dist = std::sqrt(std::max(d.norm2(), 1e-16));
        const double min_dist = a.radius + b.radius + 1e-3; // Small buffer
        const double overlap = min_dist - dist;

        if (overlap > 1e-6) {
          corrections++;
          max_overlap = std::max(max_overlap, overlap);

          // Push apart equally (unless one is kinematic)
          const Vec3 n = d / dist;
          const bool a_kinematic = a.kinematic || a.mass <= 0.0;
          const bool b_kinematic = b.kinematic || b.mass <= 0.0;

          if (!a_kinematic && !b_kinematic) {
            const Vec3 correction = n * (overlap * 0.5);
            a.position -= correction;
            b.position += correction;
          } else if (!a_kinematic) {
            a.position -= n * overlap;
          } else if (!b_kinematic) {
            b.position += n * overlap;
          }
        }
      }
    }

    if (max_overlap < 1e-6) {
      MINERVA_LOG("  Resolved in %d iterations\n", iter + 1);
      return;
    }

    if ((iter + 1) % 20 == 0) {
      MINERVA_LOG("  Iteration %d: %d corrections, max overlap = %.6f\n",
                  iter + 1, corrections, max_overlap);
    }
  }

  MINERVA_LOG("  Warning: Did not fully converge after %d iterations\n", max_iterations);
}

} // namespace minerva
