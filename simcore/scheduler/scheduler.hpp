#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace minerva {

struct World; // forward decl

// Base interface for systems/modules
struct ISystem {
  virtual ~ISystem() = default;
  virtual const char* name() const = 0;
  virtual void step(World& world, double dt) = 0;
};

// Simple scheduler with ordered systems and substeps per system
class Scheduler {
public:
  struct Entry {
    std::unique_ptr<ISystem> system;
    int substeps{1};
  };

  void add(std::unique_ptr<ISystem> sys, int substeps=1){
    entries_.push_back(Entry{std::move(sys), substeps});
  }

  void tick(World& world, double dt){
    for (auto& e : entries_){
      const double local_dt = dt / static_cast<double>(e.substeps);
      for (int s=0; s<e.substeps; ++s){
        e.system->step(world, local_dt);
      }
    }
  }

  std::vector<Entry>& entries(){ return entries_; }

private:
  std::vector<Entry> entries_;
};

} // namespace minerva
