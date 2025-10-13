#pragma once
#include "simcore/math/vec3.hpp"

#include <vector>

namespace minerva {

struct Particle {
  Vec3 position{0,0,0};
  Vec3 velocity{0,0,0};
  double mass{1.0};
};

struct ParticleSet {
  std::vector<Particle> data;
  void reserve(std::size_t n){ data.reserve(n); }
  std::size_t size() const { return data.size(); }
  Particle& operator[](std::size_t i){ return data[i]; }
  const Particle& operator[](std::size_t i) const { return data[i]; }
  void push(const Particle& p){ data.push_back(p); }
};

} // namespace minerva
