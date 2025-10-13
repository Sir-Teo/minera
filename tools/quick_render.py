#!/usr/bin/env python3
"""
Quick video rendering using matplotlib (no ParaView required).
Reads CSV output and generates MP4 videos.
"""

import os
import sys
import glob
import csv
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter
from mpl_toolkits.mplot3d import Axes3D


def read_csv_frame(filename):
    """Read positions and velocities from CSV file."""
    positions = []
    velocities = []
    radii = []

    with open(filename, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                pos = [float(row['x']), float(row['y']), float(row['z'])]
                vel = [float(row['vx']), float(row['vy']), float(row['vz'])]
                r = float(row.get('radius', 0.2))

                positions.append(pos)
                velocities.append(vel)
                radii.append(r)
            except (KeyError, ValueError):
                continue

    return np.array(positions), np.array(velocities), np.array(radii)


def render_example(example_dir, output_video, fps=30, max_frames=300):
    """Render an example to video."""
    print(f"\n  Rendering: {example_dir}")

    # Find CSV files
    csv_files = sorted(glob.glob(f"{example_dir}/*_rb_*.csv"))
    if not csv_files:
        csv_files = sorted(glob.glob(f"{example_dir}/*_md_*.csv"))

    if not csv_files:
        print(f"    No CSV files found")
        return False

    # Limit number of frames
    csv_files = csv_files[:max_frames]

    if len(csv_files) < 10:
        print(f"    Too few frames ({len(csv_files)})")
        return False

    print(f"    Found {len(csv_files)} frames")

    # Read all frames to determine bounds
    all_positions = []
    for csv_file in csv_files:
        pos, _, _ = read_csv_frame(csv_file)
        if len(pos) > 0:
            all_positions.append(pos)

    if not all_positions:
        print(f"    No valid data")
        return False

    # Compute global bounds
    all_pos_flat = np.vstack(all_positions)
    mins = all_pos_flat.min(axis=0)
    maxs = all_pos_flat.max(axis=0)
    center = (mins + maxs) / 2
    extent = (maxs - mins).max()

    # Set up plot
    fig = plt.figure(figsize=(12, 9), facecolor='#0a0a0f')
    ax = fig.add_subplot(111, projection='3d', facecolor='#0a0a0f')

    # Set limits with padding
    padding = extent * 0.2
    ax.set_xlim(center[0] - extent/2 - padding, center[0] + extent/2 + padding)
    ax.set_ylim(center[1] - extent/2 - padding, center[1] + extent/2 + padding)
    ax.set_zlim(center[2] - extent/2 - padding, center[2] + extent/2 + padding)

    ax.set_xlabel('X', color='white')
    ax.set_ylabel('Y', color='white')
    ax.set_zlabel('Z', color='white')
    ax.tick_params(colors='white')
    ax.grid(False)
    ax.xaxis.pane.fill = False
    ax.yaxis.pane.fill = False
    ax.zaxis.pane.fill = False

    # Camera angle
    ax.view_init(elev=20, azim=45)

    scatter = None

    def update(frame_idx):
        nonlocal scatter

        pos, vel, radii = read_csv_frame(csv_files[frame_idx])

        if len(pos) == 0:
            return scatter,

        # Color by velocity magnitude
        vel_mag = np.linalg.norm(vel, axis=1)
        vel_mag = np.clip(vel_mag, 0, 10)

        # Calculate proper sphere sizes in screen coordinates
        # Matplotlib's scatter size is in points^2, need to scale based on data range
        # Get axis range to convert data units to points
        x_range = ax.get_xlim()[1] - ax.get_xlim()[0]
        y_range = ax.get_ylim()[1] - ax.get_ylim()[0]
        z_range = ax.get_zlim()[1] - ax.get_zlim()[0]
        avg_range = (x_range + y_range + z_range) / 3.0

        # Figure size in inches, DPI to get figure size in pixels
        fig_width_inches = fig.get_figwidth()
        fig_dpi = fig.get_dpi()
        fig_width_pixels = fig_width_inches * fig_dpi

        # Scale factor: points per data unit (rough approximation)
        # This makes sphere visual size match their actual radius in data coordinates
        scale_factor = fig_width_pixels / avg_range if avg_range > 0 else 1.0
        scale_factor *= 0.4  # Scale down slightly for better visualization

        # Size in points^2 for scatter plot
        sizes = (radii * scale_factor) ** 2

        if scatter is not None:
            scatter.remove()

        scatter = ax.scatter(
            pos[:, 0], pos[:, 1], pos[:, 2],
            c=vel_mag,
            s=sizes,
            cmap='turbo',
            alpha=0.9,
            edgecolors='none',
            linewidths=0
        )

        return scatter,

    # Create animation
    print(f"    Creating animation...")
    anim = FuncAnimation(
        fig, update,
        frames=len(csv_files),
        interval=1000/fps,
        blit=True
    )

    # Save video
    print(f"    Saving video...")
    writer = FFMpegWriter(fps=fps, bitrate=2000, codec='libx264')
    anim.save(output_video, writer=writer, dpi=100)

    plt.close(fig)

    print(f"    ✓ Saved: {output_video}")
    return True


def main():
    print("=" * 60)
    print("Quick Video Rendering (matplotlib)")
    print("=" * 60)

    examples = [
        "avalanche", "cradle", "gas", "orbital", "domino",
        "fountain", "vortex", "gravity_well", "crystal", "wave",
        "collision", "highdrop", "multiscale"
    ]

    output_dir = "docs/assets/videos"
    os.makedirs(output_dir, exist_ok=True)

    rendered = 0

    for example in examples:
        example_dir = f"output/{example}"

        if not os.path.exists(example_dir):
            print(f"\nSkipping {example}: directory not found")
            continue

        output_video = f"{output_dir}/{example}.mp4"

        try:
            if render_example(example_dir, output_video, fps=30, max_frames=200):
                rendered += 1
        except Exception as e:
            print(f"    Error: {e}")
            continue

    print(f"\n{'='*60}")
    print(f"Complete! Rendered {rendered} videos")
    print(f"Videos saved to: {output_dir}")
    print(f"{'='*60}")


if __name__ == '__main__':
    try:
        import matplotlib
        import numpy
    except ImportError:
        print("Error: matplotlib and numpy required")
        print("Install with: pip3 install matplotlib numpy")
        sys.exit(1)

    main()
