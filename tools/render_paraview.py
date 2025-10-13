#!/usr/bin/env python3
"""
Minerva ParaView Rendering Script

Automatically renders images and videos from Minerva VTK output using ParaView.
This script uses ParaView's Python API (pvpython) for batch rendering.

Usage:
    # Generate images for all frames
    pvpython tools/render_paraview.py --images

    # Generate video
    pvpython tools/render_paraview.py --video

    # Custom settings
    pvpython tools/render_paraview.py --images --output render/ --size 1920 1080
"""

import argparse
import os
import sys
import glob

try:
    from paraview.simple import *
except ImportError:
    print("Error: ParaView Python module not found!")
    print("\nThis script must be run with pvpython (ParaView's Python interpreter):")
    print("  pvpython tools/render_paraview.py [options]")
    print("\nInstall ParaView:")
    print("  brew install --cask paraview")
    print("\nThen pvpython will be available at:")
    print("  /Applications/ParaView-*.app/Contents/bin/pvpython")
    sys.exit(1)


def find_pvd_files(output_dir="output", prefix="minerva"):
    """Find PVD collection files."""
    rb_pvd = os.path.join(output_dir, f"{prefix}_rb.pvd")
    md_pvd = os.path.join(output_dir, f"{prefix}_md.pvd")

    files = {}
    if os.path.exists(rb_pvd):
        files['rb'] = rb_pvd
    if os.path.exists(md_pvd):
        files['md'] = md_pvd

    return files


def setup_rendering(view, camera_position=None):
    """Configure rendering settings."""
    view.ViewSize = [1920, 1080]
    view.OrientationAxesVisibility = 1
    view.Background = [0.95, 0.95, 0.95]  # Light gray background

    if camera_position:
        view.CameraPosition = camera_position['position']
        view.CameraFocalPoint = camera_position['focal_point']
        view.CameraViewUp = camera_position['view_up']


def render_rigid_bodies(pvd_file, output_dir, image_format='png', frame_rate=30):
    """Render rigid body particles."""
    print(f"\nRendering rigid bodies from {pvd_file}")

    # Disable automatic camera reset
    paraview.simple._DisableFirstRenderCameraReset()

    # Load data
    reader = PVDReader(FileName=pvd_file)

    # Create view
    view = CreateView('RenderView')
    view.ViewSize = [1920, 1080]
    view.OrientationAxesVisibility = 1
    view.Background = [0.95, 0.95, 0.95]

    # Show data
    display = Show(reader, view)

    # Configure display - use Point Gaussian for spheres
    display.Representation = 'Point Gaussian'
    display.GaussianRadius = 0.25  # Match sphere radius
    display.ShaderPreset = 'Sphere'
    display.Emissive = 0

    # Color by velocity magnitude
    ColorBy(display, ('POINTS', 'velocity', 'Magnitude'))

    # Get color transfer function
    velocityLUT = GetColorTransferFunction('velocity')
    velocityLUT.ApplyPreset('Cool to Warm', True)

    # Add color bar
    velocityLUTColorBar = GetScalarBar(velocityLUT, view)
    velocityLUTColorBar.Title = 'Velocity'
    velocityLUTColorBar.ComponentTitle = 'Magnitude'
    velocityLUTColorBar.Visibility = 1

    display.SetScalarBarVisibility(view, True)

    # Compute bounds across ALL timesteps for proper camera framing
    timesteps = reader.TimestepValues
    if len(timesteps) > 1:
        # Sample several timesteps to get full spatial extent
        sample_indices = [0, len(timesteps)//4, len(timesteps)//2, 3*len(timesteps)//4, len(timesteps)-1]
        bounds_min = [float('inf')] * 3
        bounds_max = [float('-inf')] * 3

        for idx in sample_indices:
            view.ViewTime = timesteps[idx]
            Render(view)
            data_info = reader.GetDataInformation()
            bounds = data_info.DataInformation.GetBounds()

            bounds_min[0] = min(bounds_min[0], bounds[0])
            bounds_max[0] = max(bounds_max[0], bounds[1])
            bounds_min[1] = min(bounds_min[1], bounds[2])
            bounds_max[1] = max(bounds_max[1], bounds[3])
            bounds_min[2] = min(bounds_min[2], bounds[4])
            bounds_max[2] = max(bounds_max[2], bounds[5])

        # Reset to first frame
        view.ViewTime = timesteps[0]

        # Manually set camera to encompass full bounds
        center = [(bounds_min[i] + bounds_max[i]) / 2 for i in range(3)]
        extent = [(bounds_max[i] - bounds_min[i]) for i in range(3)]
        max_extent = max(extent)

        camera = GetActiveCamera()
        camera.SetFocalPoint(center[0], center[1], center[2])

        # Position camera at a distance to fit all data
        distance = max_extent * 2.5
        camera.SetPosition(center[0] + distance*0.5, center[1] + distance*0.6, center[2] + distance*0.8)
        camera.SetViewUp(0, 1, 0)
    else:
        # Single timestep - use regular ResetCamera
        view.ResetCamera()
        camera = GetActiveCamera()
        camera.Elevation(20)
        camera.Azimuth(30)
        view.CameraViewUp = [0, 1, 0]

    return reader, view, display


def render_md_particles(pvd_file, output_dir, image_format='png', frame_rate=30):
    """Render MD particles."""
    print(f"\nRendering MD particles from {pvd_file}")

    paraview.simple._DisableFirstRenderCameraReset()

    reader = PVDReader(FileName=pvd_file)
    view = CreateView('RenderView')
    view.ViewSize = [1920, 1080]
    view.OrientationAxesVisibility = 1
    view.Background = [0.95, 0.95, 0.95]

    display = Show(reader, view)

    # Use Point Gaussian for better particle visualization
    display.Representation = 'Point Gaussian'
    display.GaussianRadius = 0.15
    display.ShaderPreset = 'Sphere'

    # Color by velocity magnitude
    ColorBy(display, ('POINTS', 'velocity', 'Magnitude'))

    velocityLUT = GetColorTransferFunction('velocity')
    velocityLUT.ApplyPreset('Jet', True)

    velocityLUTColorBar = GetScalarBar(velocityLUT, view)
    velocityLUTColorBar.Title = 'Velocity'
    velocityLUTColorBar.ComponentTitle = 'Magnitude'
    velocityLUTColorBar.Visibility = 1

    display.SetScalarBarVisibility(view, True)

    # Reset camera to fit ALL data
    view.ResetCamera()

    # Adjust camera angle
    camera = GetActiveCamera()
    camera.Elevation(15)
    camera.Azimuth(45)
    view.CameraViewUp = [0, 1, 0]

    return reader, view, display


def generate_images(pvd_files, output_dir="output/frames", size=(1920, 1080)):
    """Generate images for all frames."""
    os.makedirs(output_dir, exist_ok=True)

    for data_type, pvd_file in pvd_files.items():
        print(f"\nGenerating images for {data_type}...")

        if data_type == 'rb':
            reader, view, display = render_rigid_bodies(pvd_file, output_dir)
        else:
            reader, view, display = render_md_particles(pvd_file, output_dir)

        view.ViewSize = list(size)

        # Get timesteps
        timesteps = reader.TimestepValues
        num_frames = len(timesteps)

        print(f"  Found {num_frames} timesteps")

        # Render each frame
        for i, timestep in enumerate(timesteps):
            view.ViewTime = timestep

            output_file = os.path.join(output_dir, f"{data_type}_frame_{i:06d}.png")
            SaveScreenshot(output_file, view, ImageResolution=list(size))

            if (i + 1) % 10 == 0:
                print(f"    Rendered {i + 1}/{num_frames} frames...")

        print(f"  ✓ Saved {num_frames} images to {output_dir}/")

        Delete(reader)
        Delete(view)


def generate_video(pvd_files, output_dir="output", fps=30, size=(1920, 1080)):
    """Generate videos using ParaView's animation export."""
    for data_type, pvd_file in pvd_files.items():
        print(f"\nGenerating video for {data_type}...")

        if data_type == 'rb':
            reader, view, display = render_rigid_bodies(pvd_file, output_dir)
        else:
            reader, view, display = render_md_particles(pvd_file, output_dir)

        view.ViewSize = list(size)

        timesteps = reader.TimestepValues
        num_frames = len(timesteps)

        print(f"  {num_frames} frames at {fps} FPS = {num_frames/fps:.1f} seconds")

        # Create animation scene
        animation = GetAnimationScene()
        animation.UpdateAnimationUsingDataTimeSteps()

        # Set up video writer
        output_file = os.path.join(output_dir, f"{data_type}_animation.avi")

        # Export animation
        SaveAnimation(output_file, view,
                     ImageResolution=list(size),
                     FrameRate=fps,
                     FrameWindow=[0, num_frames-1])

        print(f"  ✓ Video saved to {output_file}")
        print(f"    (Note: Convert to MP4 with: ffmpeg -i {output_file} -c:v libx264 {output_file.replace('.avi', '.mp4')})")

        Delete(reader)
        Delete(view)


def main():
    parser = argparse.ArgumentParser(
        description='Automatically render Minerva simulation using ParaView',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('--images', action='store_true',
                       help='Generate images for all frames')
    parser.add_argument('--video', action='store_true',
                       help='Generate video animation')
    parser.add_argument('--input', '-i', default='output',
                       help='Input directory with PVD files (default: output)')
    parser.add_argument('--output', '-o', default='output/renders',
                       help='Output directory (default: output/renders)')
    parser.add_argument('--size', nargs=2, type=int, default=[1920, 1080],
                       metavar=('WIDTH', 'HEIGHT'),
                       help='Image/video resolution (default: 1920 1080)')
    parser.add_argument('--fps', type=int, default=30,
                       help='Video framerate (default: 30)')
    parser.add_argument('--prefix', default='minerva',
                       help='PVD file prefix (default: minerva)')

    args = parser.parse_args()

    print("Minerva ParaView Rendering")
    print("=" * 50)
    print(f"Input: {args.input}")
    print(f"Output: {args.output}")

    # Find PVD files
    pvd_files = find_pvd_files(args.input, args.prefix)

    if not pvd_files:
        print(f"\nError: No PVD files found in {args.input}")
        print("Run the simulation first to generate VTK output")
        return 1

    print(f"\nFound PVD files: {', '.join(pvd_files.keys())}")

    if args.images:
        generate_images(pvd_files, args.output, tuple(args.size))
    elif args.video:
        generate_video(pvd_files, args.output, args.fps, tuple(args.size))
    else:
        print("\nPlease specify --images or --video")
        print("Use --help for more information")
        return 1

    print("\n✓ Rendering complete!")
    return 0


if __name__ == '__main__':
    sys.exit(main())
