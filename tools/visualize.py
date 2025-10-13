#!/usr/bin/env python3
"""
Minerva Visualization Tool

Generates images and videos from Minerva simulation CSV output.
Simple, standalone tool that works on any system with Python + matplotlib.

Usage:
    python tools/visualize.py                    # Interactive: visualize latest simulation
    python tools/visualize.py --video            # Generate MP4 video
    python tools/visualize.py --frames 10        # Generate first 10 frame images
    python tools/visualize.py --input output/    # Specify custom output directory
"""

import argparse
import glob
import os
import sys
from pathlib import Path

try:
    import numpy as np
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    from matplotlib.animation import FFMpegWriter
    from mpl_toolkits.mplot3d import Axes3D
except ImportError as e:
    print(f"Error: Missing required package: {e}")
    print("\nPlease install required packages:")
    print("  pip install numpy matplotlib")
    print("\nFor video generation, also install ffmpeg:")
    print("  brew install ffmpeg  # on macOS")
    sys.exit(1)


def find_csv_files(output_dir, prefix="minerva"):
    """Find all rigid body and MD particle CSV files."""
    rb_pattern = os.path.join(output_dir, f"{prefix}_rb_*.csv")
    md_pattern = os.path.join(output_dir, f"{prefix}_md_*.csv")

    rb_files = sorted(glob.glob(rb_pattern))
    md_files = sorted(glob.glob(md_pattern))

    return rb_files, md_files


def load_csv(filename):
    """Load particle data from CSV file."""
    try:
        data = np.genfromtxt(filename, delimiter=',', names=True)
        return data
    except Exception as e:
        print(f"Warning: Could not load {filename}: {e}")
        return None


def plot_frame(rb_data, md_data, frame_num, save_path=None, show=False):
    """Generate a single frame visualization."""
    fig = plt.figure(figsize=(12, 5))

    # Plot rigid bodies
    ax1 = fig.add_subplot(121, projection='3d')
    if rb_data is not None and len(rb_data) > 0:
        ax1.scatter(rb_data['x'], rb_data['y'], rb_data['z'],
                   c='blue', s=200*rb_data['radius']**2, alpha=0.6,
                   edgecolors='darkblue', linewidth=0.5)
        ax1.set_xlabel('X')
        ax1.set_ylabel('Y')
        ax1.set_zlabel('Z')
        ax1.set_title(f'Rigid Bodies (n={len(rb_data)})')

        # Set consistent bounds
        all_x = rb_data['x']
        all_y = rb_data['y']
        all_z = rb_data['z']

        margin = 1.0
        ax1.set_xlim([all_x.min() - margin, all_x.max() + margin])
        ax1.set_ylim([-0.5, all_y.max() + margin])
        ax1.set_zlim([all_z.min() - margin, all_z.max() + margin])
    else:
        ax1.set_title('Rigid Bodies (no data)')

    # Plot MD particles
    ax2 = fig.add_subplot(122, projection='3d')
    if md_data is not None and len(md_data) > 0:
        # Color by velocity magnitude
        v_mag = np.sqrt(md_data['vx']**2 + md_data['vy']**2 + md_data['vz']**2)
        scatter = ax2.scatter(md_data['x'], md_data['y'], md_data['z'],
                             c=v_mag, s=50, alpha=0.8, cmap='hot',
                             edgecolors='black', linewidth=0.3)
        plt.colorbar(scatter, ax=ax2, label='Velocity', shrink=0.5)
        ax2.set_xlabel('X')
        ax2.set_ylabel('Y')
        ax2.set_zlabel('Z')
        ax2.set_title(f'MD Particles (n={len(md_data)})')

        # Set consistent bounds
        all_x = md_data['x']
        all_y = md_data['y']
        all_z = md_data['z']

        margin = 1.0
        ax2.set_xlim([all_x.min() - margin, all_x.max() + margin])
        ax2.set_ylim([all_y.min() - margin, all_y.max() + margin])
        ax2.set_zlim([all_z.min() - margin, all_z.max() + margin])
    else:
        ax2.set_title('MD Particles (no data)')

    fig.suptitle(f'Minerva Simulation - Frame {frame_num}', fontsize=14, fontweight='bold')
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved: {save_path}")

    if show:
        plt.show()

    plt.close(fig)


def generate_images(output_dir, prefix="minerva", max_frames=None, output_subdir="frames"):
    """Generate PNG images for all frames."""
    rb_files, md_files = find_csv_files(output_dir, prefix)

    if not rb_files and not md_files:
        print(f"No CSV files found in {output_dir}")
        return []

    num_frames = max(len(rb_files), len(md_files))
    if max_frames:
        num_frames = min(num_frames, max_frames)

    # Create output directory for frames
    frames_dir = os.path.join(output_dir, output_subdir)
    os.makedirs(frames_dir, exist_ok=True)

    print(f"Generating {num_frames} frame images...")

    image_files = []
    for i in range(num_frames):
        rb_data = load_csv(rb_files[i]) if i < len(rb_files) else None
        md_data = load_csv(md_files[i]) if i < len(md_files) else None

        output_path = os.path.join(frames_dir, f"frame_{i:06d}.png")
        plot_frame(rb_data, md_data, i, save_path=output_path)
        image_files.append(output_path)

    print(f"✓ Generated {len(image_files)} images in {frames_dir}/")
    return image_files


def generate_video(output_dir, prefix="minerva", fps=30, output_name="minerva_simulation.mp4"):
    """Generate MP4 video from CSV data."""
    rb_files, md_files = find_csv_files(output_dir, prefix)

    if not rb_files and not md_files:
        print(f"No CSV files found in {output_dir}")
        return None

    num_frames = max(len(rb_files), len(md_files))
    print(f"Generating video from {num_frames} frames at {fps} FPS...")

    # Set up video writer
    output_path = os.path.join(output_dir, output_name)
    fig = plt.figure(figsize=(12, 5))

    writer = FFMpegWriter(fps=fps, metadata=dict(artist='Minerva Physics Engine'),
                          bitrate=5000, codec='libx264')

    with writer.saving(fig, output_path, dpi=150):
        for i in range(num_frames):
            rb_data = load_csv(rb_files[i]) if i < len(rb_files) else None
            md_data = load_csv(md_files[i]) if i < len(md_files) else None

            fig.clear()

            # Create subplots
            ax1 = fig.add_subplot(121, projection='3d')
            ax2 = fig.add_subplot(122, projection='3d')

            # Plot rigid bodies
            if rb_data is not None and len(rb_data) > 0:
                ax1.scatter(rb_data['x'], rb_data['y'], rb_data['z'],
                           c='blue', s=200*rb_data['radius']**2, alpha=0.6,
                           edgecolors='darkblue', linewidth=0.5)
                ax1.set_xlabel('X')
                ax1.set_ylabel('Y')
                ax1.set_zlabel('Z')
                ax1.set_title(f'Rigid Bodies (n={len(rb_data)})')

                all_x = rb_data['x']
                all_y = rb_data['y']
                all_z = rb_data['z']
                margin = 1.0
                ax1.set_xlim([all_x.min() - margin, all_x.max() + margin])
                ax1.set_ylim([-0.5, all_y.max() + margin])
                ax1.set_zlim([all_z.min() - margin, all_z.max() + margin])

            # Plot MD particles
            if md_data is not None and len(md_data) > 0:
                v_mag = np.sqrt(md_data['vx']**2 + md_data['vy']**2 + md_data['vz']**2)
                scatter = ax2.scatter(md_data['x'], md_data['y'], md_data['z'],
                                     c=v_mag, s=50, alpha=0.8, cmap='hot',
                                     edgecolors='black', linewidth=0.3)
                ax2.set_xlabel('X')
                ax2.set_ylabel('Y')
                ax2.set_zlabel('Z')
                ax2.set_title(f'MD Particles (n={len(md_data)})')

                all_x = md_data['x']
                all_y = md_data['y']
                all_z = md_data['z']
                margin = 1.0
                ax2.set_xlim([all_x.min() - margin, all_x.max() + margin])
                ax2.set_ylim([all_y.min() - margin, all_y.max() + margin])
                ax2.set_zlim([all_z.min() - margin, all_z.max() + margin])

            fig.suptitle(f'Minerva Simulation - Frame {i}/{num_frames}',
                        fontsize=14, fontweight='bold')
            plt.tight_layout()

            writer.grab_frame()

            if (i + 1) % 10 == 0:
                print(f"  Processed {i + 1}/{num_frames} frames...")

    plt.close(fig)
    print(f"✓ Video saved to {output_path}")
    print(f"  Duration: {num_frames/fps:.1f} seconds at {fps} FPS")
    return output_path


def main():
    parser = argparse.ArgumentParser(
        description='Visualize Minerva simulation data',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('--input', '-i', default='output',
                       help='Input directory containing CSV files (default: output)')
    parser.add_argument('--prefix', '-p', default='minerva',
                       help='CSV file prefix (default: minerva)')
    parser.add_argument('--video', '-v', action='store_true',
                       help='Generate MP4 video')
    parser.add_argument('--frames', '-f', type=int, metavar='N',
                       help='Generate N frame images (default: all)')
    parser.add_argument('--fps', type=int, default=30,
                       help='Video framerate (default: 30)')
    parser.add_argument('--output', '-o',
                       help='Output video filename (default: minerva_simulation.mp4)')

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: Directory '{args.input}' not found")
        print("Run the simulation first to generate CSV output")
        return 1

    print(f"Minerva Visualization Tool")
    print(f"{'='*50}")
    print(f"Input directory: {args.input}")

    if args.video:
        output_name = args.output if args.output else "minerva_simulation.mp4"
        generate_video(args.input, args.prefix, args.fps, output_name)
    elif args.frames is not None:
        generate_images(args.input, args.prefix, max_frames=args.frames)
    else:
        # Default: generate all frames
        generate_images(args.input, args.prefix)

    return 0


if __name__ == '__main__':
    sys.exit(main())
