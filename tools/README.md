# Minerva Visualization Tools

Automated rendering tools for Minerva simulation output.

## Quick Start

After running a simulation:

```bash
# Generate images for all frames
./tools/render images

# Generate video animation
./tools/render video

# Custom settings
./tools/render images --output my_renders --size 3840x2160
./tools/render video --fps 60
```

## Requirements

### ParaView (Recommended)
For high-quality 3D rendering with automatic camera setup:

```bash
brew install --cask paraview
```

The `render` script automatically finds ParaView's pvpython.

### Alternative: Python + matplotlib
For simple 2D plots without ParaView:

```bash
pip install numpy matplotlib
python tools/visualize.py --video
```

## Tools

### `render` - ParaView Batch Renderer (Main Tool)

Automatically renders high-quality images/videos using ParaView.

**Features:**
- Automatic ParaView detection
- Pre-configured camera angles and lighting
- Point Gaussian spheres for realistic particles
- Color-coded velocity visualization
- Batch processing of all frames

**Usage:**
```bash
./tools/render images          # Generate PNG images
./tools/render video           # Generate AVI animation
./tools/render help            # Show help
```

**Output:**
- Images: `output/renders/rb_frame_*.png`, `md_frame_*.png`
- Videos: `output/rb_animation.avi`, `md_animation.avi`

**Convert to MP4:**
```bash
ffmpeg -i output/rb_animation.avi -c:v libx264 output/rb_animation.mp4
```

### `render_paraview.py` - ParaView Python Script

Low-level ParaView scripting for custom rendering.

**Usage:**
```bash
pvpython tools/render_paraview.py --images
pvpython tools/render_paraview.py --video --fps 60
```

### `visualize.py` - Matplotlib Visualizer

Simple Python-based visualization without ParaView.

**Features:**
- Works on any system with Python
- 2D/3D matplotlib plots
- Direct video generation with ffmpeg

**Usage:**
```bash
python tools/visualize.py              # Generate all frame images
python tools/visualize.py --video      # Generate MP4 video
python tools/visualize.py --frames 50  # First 50 frames only
```

## Examples

### Basic Workflow

```bash
# 1. Run simulation
./build/minerva_demo

# 2. Generate video
./tools/render video

# 3. Convert to MP4
ffmpeg -i output/rb_animation.avi -c:v libx264 output/simulation.mp4
```

### High-Quality Renders

```bash
# 4K images at 60 FPS
./tools/render images --size 3840 2160
./tools/render video --fps 60 --size 3840 2160
```

### Custom Output Location

```bash
./tools/render images --output renders/high_quality
```

## Troubleshooting

### "pvpython not found"
Install ParaView:
```bash
brew install --cask paraview
```

Or manually specify path:
```bash
export PVPYTHON=/Applications/ParaView-5.11.0.app/Contents/bin/pvpython
./tools/render images
```

### "No PVD files found"
Run the simulation first:
```bash
./build/minerva_demo
```

### Video codec issues
If AVI playback doesn't work, convert to MP4:
```bash
brew install ffmpeg
ffmpeg -i output/rb_animation.avi -c:v libx264 output/rb.mp4
```

## Output Formats

### Images
- Format: PNG
- Resolution: 1920x1080 (default), configurable
- Location: `output/renders/`

### Videos
- Format: AVI (from ParaView), MP4 (from ffmpeg or matplotlib)
- Framerate: 30 FPS (default), configurable
- Codec: Motion JPEG (AVI), H.264 (MP4)

## Advanced Usage

### Multiple Simulations

Render different simulation outputs:

```bash
./tools/render images --input simulation1/output
./tools/render images --input simulation2/output --output renders/sim2
```

### Scripted Workflow

```bash
#!/bin/bash
# render_all.sh - Render multiple configurations

for config in config1 config2 config3; do
    echo "Rendering $config..."
    ./build/minerva_demo --config $config
    ./tools/render video --output renders/$config
done
```

## Tips

1. **Preview first frame** before rendering all:
   ```bash
   python tools/visualize.py --frames 1
   ```

2. **Parallel rendering** (if you have multiple simulations):
   ```bash
   ./tools/render images --input sim1/output &
   ./tools/render images --input sim2/output &
   wait
   ```

3. **Customize colors/camera** by editing `render_paraview.py`:
   - Line 80-85: Camera position
   - Line 100-105: Color schemes
   - Line 110: Particle size

## See Also

- [ParaView Documentation](https://www.paraview.org/documentation/)
- [ParaView Python API](https://kitware.github.io/paraview-docs/latest/python/)
- [Minerva Main README](../README.md)
