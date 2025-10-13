# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Minerva is a lightweight, scalable multi-physics simulation engine designed for Mac (Apple Silicon/Intel). It covers macro-scale (rigid bodies, fluids) and micro-scale (molecular dynamics) physics with a modular architecture that enables easy extension and coupling between different physics domains.

## Build System

### Prerequisites (Mac)
```bash
xcode-select --install
brew install cmake ninja
```

### Building
```bash
# Standard build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C build

# With sanitizers (recommended for development)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMINERVA_ENABLE_SANITIZERS=ON
ninja -C build

# Run the demo
./build/minerva_demo
```

### Build Configuration
- C++20 standard is required
- Sanitizers (ASan/UBSan) are enabled by default in Debug/RelWithDebInfo builds via `MINERVA_ENABLE_SANITIZERS` option
- Compiler flags enforce strict warnings: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`
- Exceptions are disabled on Apple platforms with `-fno-exceptions`

## Architecture

### Core Design Principles

1. **Modular Plugin System**: Physics modules (rigid bodies, MD, SPH, etc.) implement the `ISystem` interface and plug into the scheduler
2. **Separation of Concerns**: Core (`simcore/`) provides infrastructure; modules (`modules/`) implement domain-specific physics
3. **Data-Oriented Design**: State containers (RigidBody, Particle, etc.) are designed for cache-friendly access
4. **Scheduler-Based Execution**: The `Scheduler` orchestrates system updates with configurable substepping per system

### Directory Structure

```
simcore/           # Core engine (no dependencies on physics modules)
├── base/          # Logging utilities (MINERVA_LOG macro)
├── math/          # Vec3 math primitives
├── state/         # Data containers: RigidBody, Particle, ParticleSet
├── scheduler/     # ISystem interface + Scheduler
├── io/            # Output writers (CSV, VTK)
└── world.hpp      # Top-level World container + orchestration

modules/           # Physics implementations
├── rb/            # Rigid body system (spheres + ground plane collisions)
└── md/            # Molecular dynamics (Lennard-Jones forces)

examples/          # Runnable demonstrations
```

### Key Components

**World**: Central simulation state containing:
- `rigid_bodies` vector for macro-scale bodies
- `md_particles` ParticleSet for micro-scale particles
- `scheduler` for orchestrating physics systems
- Global parameters: `time`, `gravity`

**ISystem Interface**: All physics modules must implement:
- `name()`: Returns system identifier
- `step(World& world, double dt)`: Advances physics by timestep dt

**Scheduler**: Manages system execution with per-system substepping. Systems execute in registration order.

## Implementation Guidelines

### Adding a New Physics Module

1. Create header in `modules/<name>/<name>_system.hpp`
2. Implement `ISystem` interface:
   ```cpp
   class MySystem final : public ISystem {
   public:
     const char* name() const override { return "MySystem"; }
     void step(World& world, double dt) override;
   private:
     // Config and internal state
   };
   ```
3. Add implementation in corresponding `.cpp` file
4. Update `CMakeLists.txt` to include new sources in `minerva_core` library
5. Instantiate and register in simulation setup (see `examples/minerva_demo.cpp`)

### Working with State Containers

- **RigidBody**: Has position, velocity, mass, radius, and kinematic flag
- **Particle**: Simpler structure with position, velocity, mass
- **ParticleSet**: Wrapper around `std::vector<Particle>` with convenience methods

Access patterns should be cache-friendly (iterate contiguously, minimize indirection).

### Coordinate Systems and Units

- Gravity defaults to `(0, -9.81, 0)` in m/s²
- Ground plane at y=0 by default (configurable in RigidBodySystemConfig)
- No automatic unit conversion - maintain consistency across modules

### Logging

Use `MINERVA_LOG(...)` macro (printf-style) which is active in debug builds only:
```cpp
MINERVA_LOG("Spawned %zu rigid bodies\n", world.rigid_bodies.size());
```

## Current Physics Implementations

### Rigid Body System (`modules/rb/`)
- Sphere primitives with ground plane collision
- Semi-implicit Euler integration
- Restitution (bounce) and crude tangential friction
- Collision detection against infinite ground plane only (sphere-sphere not yet implemented)

### MD System (`modules/md/`)
- Lennard-Jones 12-6 potential with cutoff
- Velocity-Verlet integrator
- Optional Berendsen thermostat (NVT ensemble)
- **Cell list + Verlet neighbor list** for O(N) scaling (4-5x speedup vs all-pairs)
  - Configurable skin distance and rebuild frequency
  - Automatic domain decomposition
  - Displacement-based rebuild heuristics
- No periodic boundaries yet (free boundary conditions)

### I/O System (`simcore/io/`)
- **CSVWriter**: Outputs simulation data to CSV files for easy analysis
  - Separate files for rigid bodies and MD particles
  - Includes position, velocity, mass, and type-specific properties
  - Time-series data with frame numbers

- **VTKWriter**: Outputs VTK XML format (.vtu) for ParaView visualization
  - Unstructured grid format with point data
  - Generates .pvd collection files for time-series playback
  - Supports velocity vectors, scalar fields (mass, radius)
  - Automatic rendering: `./tools/render images` or `./tools/render video`

Example usage:
```cpp
VTKWriterConfig cfg;
cfg.output_dir = "output";
cfg.prefix = "sim";
auto writer = std::make_unique<VTKWriter>(cfg);

// In simulation loop
writer->write(world, frame_number);

// At end
writer->finalize(); // Creates .pvd collection files
```

### Visualization Tools (`tools/`)
- **render**: Shell wrapper for automatic ParaView batch rendering
  - Finds ParaView installation automatically
  - Pre-configured camera angles and coloring
  - `./tools/render images` - Generate PNG frames
  - `./tools/render video` - Generate AVI animation

- **render_paraview.py**: ParaView Python script
  - Point Gaussian sphere rendering
  - Velocity-based coloring
  - Configurable resolution and framerate

- **visualize.py**: Python/matplotlib alternative (no ParaView needed)
  - 2D/3D matplotlib plots
  - Direct MP4 video generation

See `tools/README.md` for complete documentation.

## Planned Extensions (from roadmap)

Near-term high-leverage additions:
1. ✅ CSV/VTK writers for visualization in ParaView
2. ✅ Neighbor lists for MD (cell lists + Verlet skin) → 4-5× speedup achieved, scales to O(N)
3. Sphere-sphere rigid body collisions with impulse solver
4. SPH fluids module (reuse MD neighbor infrastructure)
5. Python bindings via pybind11
6. Module interface with C ABI and dynamic loading

## Development Philosophy

- **CPU-first**: Optimize for Mac CPU/threading before GPU
- **Minimal dependencies**: Header-only libs preferred (Eigen considered for future)
- **Small, testable increments**: Each module should be independently validatable
- **Performance-conscious**: Structure-of-arrays where beneficial, SIMD-friendly loops
- **Sanitizer-clean**: All code must run without ASan/UBSan errors

## Reference Documentation

The `docs/` directory contains:
- `skeleton.md`: Complete implementation skeleton with all current code
- `roadmap.md`: Detailed milestone plan and tech stack decisions
- `research.md`: Survey of multi-physics simulation architectures and comparison with existing libraries (OpenMM, LAMMPS, GROMACS, Bullet, MuJoCo, Chrono, OpenFOAM)

When implementing new features, refer to these documents for architectural guidance and implementation patterns.
