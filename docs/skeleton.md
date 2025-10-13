Fantastic—Minerva it is. Below is a clean, Mac‑friendly starter kit you can paste into a repo and build immediately. It lays down:
	•	a tiny core (state + scheduler + math),
	•	two systems: rigid bodies (spheres + ground plane) and minimal MD (Lennard‑Jones),
	•	a demo that runs both,
	•	a small CMake with sanitizer toggles.

This is intentionally small and readable, so you can iterate fast.

⸻

Directory layout

minerva/
├─ CMakeLists.txt
├─ simcore/
│  ├─ base/log.hpp
│  ├─ math/vec3.hpp
│  ├─ state/rigid_body.hpp
│  ├─ state/particle.hpp
│  ├─ scheduler/scheduler.hpp
│  └─ world.hpp
├─ modules/
│  ├─ rb/rigid_body_system.hpp
│  ├─ rb/rigid_body_system.cpp
│  ├─ md/md_system.hpp
│  └─ md/md_system.cpp
└─ examples/minerva_demo.cpp


⸻

Build prerequisites (Mac)

xcode-select --install
brew install cmake ninja


⸻

Top-level CMake

CMakeLists.txt

cmake_minimum_required(VERSION 3.20)
project(minerva LANGUAGES CXX)

option(MINERVA_ENABLE_SANITIZERS "Enable ASan/UBSan" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (APPLE)
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -fno-exceptions)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if (MINERVA_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

add_library(minerva_core
  simcore/base/log.hpp
  simcore/math/vec3.hpp
  simcore/state/rigid_body.hpp
  simcore/state/particle.hpp
  simcore/scheduler/scheduler.hpp
  simcore/world.hpp
  modules/rb/rigid_body_system.hpp
  modules/rb/rigid_body_system.cpp
  modules/md/md_system.hpp
  modules/md/md_system.cpp
)

target_include_directories(minerva_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(minerva_demo examples/minerva_demo.cpp)
target_link_libraries(minerva_demo PRIVATE minerva_core)


⸻

Core: logging

simcore/base/log.hpp

#pragma once
#include <cstdio>

#ifndef NDEBUG
  #define MINERVA_LOG(...) std::printf("[Minerva] " __VA_ARGS__)
#else
  #define MINERVA_LOG(...) do {} while(0)
#endif


⸻

Core: math

simcore/math/vec3.hpp

#pragma once
#include <cmath>
#include <algorithm>
#include <ostream>

namespace minerva {

struct Vec3 {
  double x{}, y{}, z{};
  constexpr Vec3() = default;
  constexpr Vec3(double X, double Y, double Z) : x(X), y(Y), z(Z) {}

  static constexpr Vec3 zero() { return Vec3(0.0, 0.0, 0.0); }
  static constexpr Vec3 unit_x() { return Vec3(1.0, 0.0, 0.0); }
  static constexpr Vec3 unit_y() { return Vec3(0.0, 1.0, 0.0); }
  static constexpr Vec3 unit_z() { return Vec3(0.0, 0.0, 1.0); }

  Vec3 operator+(const Vec3& b) const { return {x+b.x, y+b.y, z+b.z}; }
  Vec3 operator-(const Vec3& b) const { return {x-b.x, y-b.y, z-b.z}; }
  Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }
  Vec3 operator/(double s) const { return {x/s, y/s, z/s}; }
  Vec3& operator+=(const Vec3& b){ x+=b.x; y+=b.y; z+=b.z; return *this; }
  Vec3& operator-=(const Vec3& b){ x-=b.x; y-=b.y; z-=b.z; return *this; }
  Vec3& operator*=(double s){ x*=s; y*=s; z*=s; return *this; }

  double dot(const Vec3& b) const { return x*b.x + y*b.y + z*b.z; }
  Vec3 cross(const Vec3& b) const {
    return { y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x };
  }
  double norm2() const { return this->dot(*this); }
  double norm() const { return std::sqrt(norm2()); }
  Vec3 normalized() const { double n = norm(); return n>0?(*this)/n:*this; }
};

inline Vec3 operator*(double s, const Vec3& v) { return v*s; }
inline std::ostream& operator<<(std::ostream& os, const Vec3& v){
  return os << "(" << v.x << "," << v.y << "," << v.z << ")";
}

} // namespace minerva


⸻

Core: state types

simcore/state/rigid_body.hpp

#pragma once
#include "simcore/math/vec3.hpp"

namespace minerva {

// Minimal rigid body as a sphere for now
struct RigidBody {
  Vec3 position{0,0,0};
  Vec3 velocity{0,0,0};
  double mass{1.0};
  double radius{0.5};
  bool   kinematic{false}; // if true, we ignore dynamics
};

} // namespace minerva

simcore/state/particle.hpp

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


⸻

Core: scheduler & world

simcore/scheduler/scheduler.hpp

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

simcore/world.hpp

#pragma once
#include "simcore/state/rigid_body.hpp"
#include "simcore/state/particle.hpp"
#include "simcore/math/vec3.hpp"
#include "simcore/scheduler/scheduler.hpp"
#include <vector>
#include <cstdint>

namespace minerva {

struct World {
  // Global settings
  double time{0.0};
  Vec3 gravity{0.0, -9.81, 0.0};

  // State containers
  std::vector<RigidBody> rigid_bodies;
  ParticleSet md_particles;

  // Orchestration
  Scheduler scheduler;

  void step(double dt){
    scheduler.tick(*this, dt);
    time += dt;
  }
};

} // namespace minerva


⸻

Modules: Rigid bodies (spheres + ground plane)

modules/rb/rigid_body_system.hpp

#pragma once
#include "simcore/scheduler/scheduler.hpp"

namespace minerva {

struct RigidBodySystemConfig {
  double restitution{0.5};  // bounce
  double friction{0.3};     // not used yet
  double ground_y{0.0};     // y = ground plane
};

class RigidBodySystem final : public ISystem {
public:
  explicit RigidBodySystem(const RigidBodySystemConfig& cfg) : cfg_(cfg) {}
  const char* name() const override { return "RigidBodySystem"; }
  void step(World& world, double dt) override;

private:
  RigidBodySystemConfig cfg_;
};

} // namespace minerva

modules/rb/rigid_body_system.cpp

#include "modules/rb/rigid_body_system.hpp"
#include "simcore/world.hpp"
#include "simcore/base/log.hpp"
#include <algorithm>

namespace minerva {

void RigidBodySystem::step(World& world, double dt){
  // Integrate + ground-plane collisions (very simple)
  for (auto& rb : world.rigid_bodies){
    if (rb.kinematic || rb.mass <= 0.0) continue;

    // Semi-implicit Euler
    rb.velocity += world.gravity * dt;
    rb.position += rb.velocity * dt;

    // Ground plane collision at y = ground_y (penalty-free impulse style)
    const double bottom = rb.position.y - rb.radius;
    if (bottom < cfg_.ground_y){
      // Correct position
      rb.position.y = cfg_.ground_y + rb.radius;

      // Reflect velocity with restitution on normal component (n = +Y)
      double vn = rb.velocity.y;     // normal component along +Y
      if (vn < 0.0){
        rb.velocity.y = -cfg_.restitution * vn;
        // crude tangential damping to mimic frictional losses
        rb.velocity.x *= 0.98;
        rb.velocity.z *= 0.98;
      }
    }
  }
}

} // namespace minerva


⸻

Modules: Minimal MD (Lennard‑Jones)

modules/md/md_system.hpp

#pragma once
#include "simcore/scheduler/scheduler.hpp"

namespace minerva {

struct MDConfig {
  double epsilon{1.0};
  double sigma{1.0};
  double rcut_sigma{2.5};  // cutoff in units of sigma
  bool   nvt{false};
  double temp{1.0};
  double tau_thermo{1.0};  // Berendsen time constant
};

class MDSystem final : public ISystem {
public:
  explicit MDSystem(const MDConfig& cfg) : cfg_(cfg) {}
  const char* name() const override { return "MDSystem"; }
  void step(World& world, double dt) override;

private:
  MDConfig cfg_;
  // simple velocity-verlet integrator
  void integrate(World& world, double dt);
};

} // namespace minerva

modules/md/md_system.cpp

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
    double dof = 3.0 * static_cast<double>(n);
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

Notes
	•	This MD is deliberately bare‑bones (no periodic boundaries yet). It’s perfect for a first “gas of particles” sanity check and gives you a template for neighbor lists later.

⸻

Example: drive both systems

examples/minerva_demo.cpp

#include "simcore/world.hpp"
#include "modules/rb/rigid_body_system.hpp"
#include "modules/md/md_system.hpp"
#include "simcore/base/log.hpp"
#include <random>
#include <iostream>

using namespace minerva;

int main(){
  World world;

  // --- Rigid bodies: spawn a small stack of spheres
  {
    std::mt19937 rng(1337);
    std::uniform_real_distribution<double> ux(-1.0, 1.0);

    const int layers = 5;
    int count = 0;
    for (int y=0; y<layers; ++y){
      for (int x=0; x<layers; ++x){
        for (int z=0; z<layers; ++z){
          RigidBody rb;
          rb.radius = 0.25;
          rb.mass   = 1.0;
          rb.position = Vec3( -0.5 + x*0.55 + 0.05*ux(rng),
                               2.0 + y*0.55,
                              -0.5 + z*0.55 + 0.05*ux(rng) );
          world.rigid_bodies.push_back(rb);
          ++count;
        }
      }
    }
    MINERVA_LOG("Spawned %zu rigid spheres\n", world.rigid_bodies.size());
  }

  // --- MD particles: small Lennard-Jones cloud
  {
    const int n_side = 6;
    const double spacing = 1.2; // loose to avoid huge forces at start
    for (int i=0; i<n_side; ++i)
      for (int j=0; j<n_side; ++j)
        for (int k=0; k<n_side; ++k){
          Particle p;
          p.mass = 1.0;
          p.position = Vec3(3.0 + i*spacing, 1.0 + j*spacing, -2.0 + k*spacing);
          world.md_particles.push(p);
        }
    MINERVA_LOG("Spawned %zu MD particles\n", world.md_particles.size());
  }

  // --- Systems
  RigidBodySystemConfig rb_cfg;
  rb_cfg.restitution = 0.5;
  rb_cfg.ground_y = 0.0;

  MDConfig md_cfg;
  md_cfg.rcut_sigma = 2.5;
  md_cfg.nvt = false; // try NVE first

  world.scheduler.add(std::make_unique<RigidBodySystem>(rb_cfg), /*substeps*/1);
  world.scheduler.add(std::make_unique<MDSystem>(md_cfg), /*substeps*/1);

  // --- Simulate
  const double dt = 1.0/120.0;  // small time step
  const int steps = 1200;

  for (int s=0; s<steps; ++s){
    world.step(dt);

    if (s % 120 == 0){
      // Print one RB and one MD particle as a quick sanity readout
      const auto& rb = world.rigid_bodies.front();
      const auto& mp = world.md_particles.data.front();
      std::cout << "t=" << world.time
                << "  RB.y=" << rb.position.y
                << "  MD.v2=" << mp.velocity.norm2()
                << "\n";
    }
  }

  MINERVA_LOG("Done. Final time: %.3f s\n", world.time);
  return 0;
}


⸻

Build & run

cd minerva
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C build
./build/minerva_demo

You should see logs each simulated second and no sanitizer complaints. The rigid spheres will drop onto the ground plane and settle; the MD particles will jiggle from LJ interactions.

⸻

What you have now
	•	World/state containers for rigid bodies and MD particles.
	•	Scheduler that can do per‑system substepping (already parameterized).
	•	RBSystem with gravity and ground collision (simple restitution).
	•	MDSystem with Velocity‑Verlet and LJ forces + optional Berendsen thermostat.
	•	Minimal logging and a clean CMake.

This is a sturdy base to expand into SPH, better collisions, neighbor lists, and a Python API.

⸻

Next small, high‑leverage additions (suggested order)
	1.	CSV/VTK writers for particles & rigid bodies (view in ParaView).
	2.	Neighbor lists for MD (cell lists + Verlet skin) → ~10–100× faster at scale.
	3.	Rigid body collisions (sphere–sphere) with impulse solver (PGS), then box shapes.
	4.	SPH fluids (reuse MD neighbor infra) → dam‑break demo.
	5.	Python bindings (pybind11) to set up scenes in 10–30 lines.
	6.	Module interface (C ABI) and dynamic loading once you’re happy with the core ABI.

If you want, I can extend this with (a) a tiny VTK writer, (b) sphere–sphere collisions, or (c) a minimal pybind11 module so you can run pip install -e . and script Minerva from Python.