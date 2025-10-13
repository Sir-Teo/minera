#pragma once
#include <string>

namespace minerva {

struct World; // forward decl

// Base interface for simulation output writers
class IWriter {
public:
  virtual ~IWriter() = default;

  // Write a single frame/timestep of simulation data
  virtual void write(const World& world, int frame_number) = 0;

  // Optional: finalize any multi-frame output
  virtual void finalize() {}
};

} // namespace minerva
