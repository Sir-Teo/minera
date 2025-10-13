# Minerva

A lightweight, scalable multi-physics simulation engine designed for Mac (Apple Silicon/Intel). Minerva covers macro-scale (rigid bodies, fluids) and micro-scale (molecular dynamics) physics with a modular architecture.

[![CI](https://github.com/USER/minera/workflows/CI/badge.svg)](https://github.com/USER/minera/actions)

## Features

- **Modular Architecture**: Physics modules plug into a flexible scheduler system
- **Rigid Body Physics**: Sphere-based rigid bodies with ground plane collisions
- **Molecular Dynamics**: Lennard-Jones potential with Velocity-Verlet integration
- **Multi-Scale Support**: Designed to handle both macro and micro physics in a unified framework
- **Performance Focused**: Cache-friendly data structures, sanitizer-clean code

## Quick Start

### Prerequisites (Mac)

```bash
xcode-select --install
brew install cmake
```

### Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Running the Demo

```bash
./build/minerva_demo
```

This will run a simulation with 125 rigid spheres falling onto a ground plane and 216 molecular dynamics particles interacting via Lennard-Jones forces.

## Project Structure

```
simcore/           # Core engine
├── base/          # Logging utilities
├── math/          # Vec3 math primitives
├── state/         # Data containers (RigidBody, Particle)
├── scheduler/     # ISystem interface + Scheduler
└── world.hpp      # Top-level World container

modules/           # Physics implementations
├── rb/            # Rigid body system
└── md/            # Molecular dynamics system

examples/          # Runnable demonstrations
```

## Development

### Build with Sanitizers (Recommended)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMINERVA_ENABLE_SANITIZERS=ON
cmake --build build
```

### Adding a New Physics Module

1. Create your module in `modules/<name>/`
2. Implement the `ISystem` interface
3. Add to `CMakeLists.txt`
4. Register with the scheduler in your simulation

See `CLAUDE.md` for detailed development guidelines.

## Planned Features

- CSV/VTK output for visualization in ParaView
- Neighbor lists for MD (10-100× speedup)
- Sphere-sphere rigid body collisions
- SPH fluid dynamics
- Python bindings via pybind11

## Architecture

Minerva uses a **scheduler-based execution model** where physics modules (systems) implement a common interface and are executed in order with configurable substepping. The `World` container holds all simulation state, and systems read/write to it during their `step()` calls.

For more details, see the [research document](docs/research.md) and [roadmap](docs/roadmap.md).

## License

This project is open source. License details to be added.

## Contributing

Contributions welcome! Please ensure code builds without warnings and passes sanitizer checks.
