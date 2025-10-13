# Minerva Examples

Demonstration programs showing different simulation scenarios using the Minerva physics engine.

## Available Examples

### 1. High Drop (`example_highdrop`)

**Description:** Large number of rigid spheres falling from significant height onto a ground plane.

**Features:**
- 960 rigid body spheres (8×15×8 grid)
- Drop height: 8 meters
- High restitution (0.7) for bouncy collisions
- Demonstrates energy dissipation and collision dynamics

**Run:**
```bash
./build/example_highdrop
./tools/render video --input output/highdrop --prefix highdrop --fps 30
```

**Output:** `output/highdrop/highdrop.mp4`

---

### 2. Large MD System (`example_md_large`)

**Description:** Large-scale molecular dynamics simulation with thermal equilibration.

**Features:**
- 512 particles (8×8×8 grid)
- Lennard-Jones interactions with neighbor lists
- NVT ensemble with Berendsen thermostat
- Target temperature: 1.5 (reduced units)
- Demonstrates O(N) performance scaling

**Run:**
```bash
./build/example_md_large
./tools/render video --input output/md_large --prefix md_large --fps 30
```

**Output:** `output/md_large/md_large.mp4`

**Performance:** Neighbor lists provide 4-5× speedup over all-pairs for this system size.

---

### 3. Multi-Scale Interaction (`example_multiscale`)

**Description:** Combined rigid body and molecular dynamics simulation in one scene.

**Features:**
- 16 large rigid body spheres falling
- 300 MD particles in a cloud
- Both physics modules running simultaneously
- Demonstrates scheduler-based multi-physics architecture

**Run:**
```bash
./build/example_multiscale
./tools/render video --input output/multiscale --prefix multiscale --fps 30
```

**Output:**
- `output/multiscale/multiscale_rb.mp4` (rigid bodies)
- `output/multiscale/multiscale_md.mp4` (MD particles)

---

### 4. Collision Cascade (`example_collision`)

**Description:** Dramatic head-on collision between multiple groups of spheres.

**Features:**
- Three groups: left (moving right), right (moving left), center (stationary)
- 6×6 spheres in each moving group, 4×4 in center
- High initial velocities (±3 m/s)
- Demonstrates momentum conservation and collision dynamics

**Run:**
```bash
./build/example_collision
./tools/render video --input output/collision --prefix collision --fps 30
```

**Output:** `output/collision/collision.mp4`

**Physics:** Watch kinetic energy transform through elastic collisions!

---

## Quick Start

Build all examples:
```bash
cmake --build build -j$(sysctl -n hw.ncpu)
```

Run all examples and generate videos:
```bash
for example in highdrop md_large multiscale collision; do
  echo "Running example_${example}..."
  ./build/example_${example}
done

# Generate videos
./tools/render video --input output/highdrop --prefix highdrop
./tools/render video --input output/md_large --prefix md_large
./tools/render video --input output/multiscale --prefix multiscale
./tools/render video --input output/collision --prefix collision
```

## Creating Your Own Example

1. Create a new `.cpp` file in `examples/`
2. Include necessary headers:
   ```cpp
   #include "simcore/world.hpp"
   #include "modules/rb/rigid_body_system.hpp"
   #include "modules/md/md_system.hpp"
   #include "simcore/io/csv_writer.hpp"
   #include "simcore/io/vtk_writer.hpp"
   ```

3. Set up your simulation:
   ```cpp
   World world;

   // Add particles/bodies
   RigidBody rb;
   rb.position = Vec3(0, 5, 0);
   rb.radius = 0.25;
   world.rigid_bodies.push_back(rb);

   // Add physics systems
   world.scheduler.add(std::make_unique<RigidBodySystem>(config), 1);

   // Set up I/O
   auto vtk_writer = std::make_unique<VTKWriter>(vtk_config);

   // Run simulation
   for (int s=0; s<steps; ++s) {
     world.step(dt);
     if (s % output_interval == 0) {
       vtk_writer->write(world, frame++);
     }
   }
   vtk_writer->finalize();
   ```

4. Add to `CMakeLists.txt`:
   ```cmake
   add_executable(my_example examples/my_example.cpp)
   target_link_libraries(my_example PRIVATE minerva_core)
   ```

5. Build and run:
   ```bash
   cmake --build build
   ./build/my_example
   ./tools/render video --input output/my_output --prefix my_prefix
   ```

## Configuration Tips

### Rigid Body System
- `restitution`: 0.0 (inelastic) to 1.0 (perfectly elastic)
- `ground_y`: Height of ground plane
- Higher `world.gravity.y` magnitude = faster falling

### MD System
- `epsilon`: Energy scale for Lennard-Jones potential
- `sigma`: Length scale (particle size)
- `rcut_sigma`: Cutoff distance in units of sigma (typically 2.5)
- `use_neighbor_list`: Enable for systems with >200 particles
- `nvt`: Enable thermostat for temperature control
- `temp`: Target temperature (reduced units)

### Performance
- Neighbor lists: Recommended for >200 MD particles
- Substepping: Use more substeps for stiff systems
- Output interval: Higher = faster simulation, fewer frames

## Visualization

All examples use the automated visualization tools:

**Generate videos:**
```bash
./tools/render video --input <output_dir> --prefix <prefix>
```

**Generate image sequences:**
```bash
./tools/render images --input <output_dir> --prefix <prefix>
```

**Custom resolution:**
```bash
./tools/render video --size 3840 2160 --fps 60
```

See [tools/README.md](../tools/README.md) for detailed visualization options.

## Example Parameters

| Example | Particles/Bodies | Simulation Time | Output Frames | Video Length |
|---------|------------------|-----------------|---------------|--------------|
| highdrop | 960 RB | 20s | 300 | 10s @ 30fps |
| md_large | 512 MD | 15s | 300 | 10s @ 30fps |
| multiscale | 16 RB + 300 MD | 20s | 300 | 10s @ 30fps |
| collision | 88 RB | 15s | 300 | 10s @ 30fps |

## Troubleshooting

**Simulation crashes:**
- Check time step size (smaller = more stable)
- Verify initial particle positions don't overlap
- For MD: ensure neighbor list domain covers all particles

**Particles explode:**
- Initial velocities too high
- MD: particles too close (use larger `spacing`)
- Enable NVT thermostat to prevent runaway energy

**Poor performance:**
- Enable neighbor lists for large MD systems
- Reduce output frequency
- Build in RelWithDebInfo or Release mode

## See Also

- [Main README](../README.md) - Project overview
- [tools/README.md](../tools/README.md) - Visualization guide
- [CLAUDE.md](../CLAUDE.md) - Development guidelines
