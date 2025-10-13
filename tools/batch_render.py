#!/usr/bin/env python3
"""
Batch render all Minerva examples to videos.

Usage:
    pvpython tools/batch_render.py
"""

import os
import sys
import glob

try:
    from paraview.simple import *
except ImportError:
    print("Error: Must run with pvpython!")
    sys.exit(1)


def render_example(pvd_file, output_video, resolution=(1280, 720), fps=30):
    """Render a single example to video."""
    print(f"\n  Processing: {os.path.basename(pvd_file)}")

    # Disable automatic camera reset
    paraview.simple._DisableFirstRenderCameraReset()

    # Load data
    reader = PVDReader(FileName=pvd_file)

    # Create view
    view = CreateView('RenderView')
    view.ViewSize = list(resolution)
    view.OrientationAxesVisibility = 0  # Disable axes for cleaner look
    view.Background = [0.1, 0.1, 0.15]  # Dark background

    # Show data
    display = Show(reader, view)

    # Configure display - Point Gaussian for spheres
    display.Representation = 'Point Gaussian'

    # Determine appropriate Gaussian radius from data
    if '_rb' in os.path.basename(pvd_file):
        display.GaussianRadius = 0.25
    else:  # MD particles
        display.GaussianRadius = 0.15

    display.ShaderPreset = 'Sphere'
    display.Emissive = 0

    # Color by velocity magnitude
    ColorBy(display, ('POINTS', 'velocity', 'Magnitude'))

    velocityLUT = GetColorTransferFunction('velocity')
    velocityLUT.ApplyPreset('Turbo', True)

    # No color bar for cleaner look
    display.SetScalarBarVisibility(view, False)

    # Get timesteps
    timesteps = reader.TimestepValues

    if len(timesteps) > 1:
        # Compute bounds across multiple timesteps
        sample_indices = [0, len(timesteps)//4, len(timesteps)//2,
                          3*len(timesteps)//4, len(timesteps)-1]
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

        # Set camera to encompass full bounds
        center = [(bounds_min[i] + bounds_max[i]) / 2 for i in range(3)]
        extent = [(bounds_max[i] - bounds_min[i]) for i in range(3)]
        max_extent = max(extent)

        camera = GetActiveCamera()
        camera.SetFocalPoint(center[0], center[1], center[2])

        # Position camera at a distance
        distance = max_extent * 2.2
        camera.SetPosition(
            center[0] + distance*0.6,
            center[1] + distance*0.5,
            center[2] + distance*0.8
        )
        camera.SetViewUp(0, 1, 0)

    print(f"  Rendering {len(timesteps)} frames...")

    # Create animation
    animation = GetAnimationScene()
    animation.UpdateAnimationUsingDataTimeSteps()

    # Save animation
    SaveAnimation(output_video, view,
                  ImageResolution=list(resolution),
                  FrameRate=fps,
                  FrameWindow=[0, len(timesteps)-1])

    print(f"  ✓ Saved: {output_video}")

    Delete(reader)
    Delete(view)


def main():
    print("=" * 60)
    print("Batch Rendering Minerva Examples")
    print("=" * 60)

    examples = [
        "avalanche", "newtons_cradle", "gas_expansion", "orbital_ring",
        "domino_chain", "fountain", "vortex", "gravity_well", "crystal", "wave",
        "collision", "highdrop", "multiscale"
    ]

    output_dir = "docs/assets/videos"
    os.makedirs(output_dir, exist_ok=True)

    rendered_count = 0

    for example in examples:
        example_dir = f"output/{example}"

        if not os.path.exists(example_dir):
            print(f"\nSkipping {example}: directory not found")
            continue

        print(f"\n{'='*60}")
        print(f"Example: {example}")
        print(f"{'='*60}")

        # Find PVD files
        pvd_files = glob.glob(f"{example_dir}/*.pvd")

        if not pvd_files:
            print(f"  No PVD files found")
            continue

        # Render each PVD file (rb or md)
        for pvd_file in pvd_files:
            output_name = f"{example}.avi"
            output_video = os.path.join(example_dir, output_name)

            try:
                render_example(pvd_file, output_video)
                rendered_count += 1

                # Note: AVI will be converted to MP4 by the shell script
                break  # Only render first PVD file per example

            except Exception as e:
                print(f"  Error rendering {pvd_file}: {e}")
                continue

    print(f"\n{'='*60}")
    print(f"Rendering complete! Rendered {rendered_count}/{len(examples)} examples")
    print(f"{'='*60}")
    print("\nNote: Use ffmpeg to convert AVI to MP4:")
    print("  ffmpeg -i input.avi -c:v libx264 -preset medium -crf 23 output.mp4")


if __name__ == '__main__':
    main()
