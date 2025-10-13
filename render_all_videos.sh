#!/bin/bash
# Render all example simulations to MP4 videos

set -e

# Find pvpython (ParaView's Python)
PVPYTHON=""
if command -v pvpython &> /dev/null; then
    PVPYTHON="pvpython"
elif [ -f "/Applications/ParaView.app/Contents/bin/pvpython" ]; then
    PVPYTHON="/Applications/ParaView.app/Contents/bin/pvpython"
else
    # Search for ParaView installations
    for app in /Applications/ParaView*.app; do
        if [ -f "$app/Contents/bin/pvpython" ]; then
            PVPYTHON="$app/Contents/bin/pvpython"
            break
        fi
    done
fi

if [ -z "$PVPYTHON" ]; then
    echo "Error: pvpython not found!"
    echo "Please install ParaView: brew install --cask paraview"
    exit 1
fi

echo "Using pvpython: $PVPYTHON"
echo ""

examples=(
  "avalanche"
  "newtons_cradle"
  "gas_expansion"
  "orbital_ring"
  "domino_chain"
  "fountain"
  "vortex"
  "gravity_well"
  "crystal"
  "wave"
  "collision"
  "highdrop"
  "multiscale"
)

mkdir -p docs/assets/videos

for example in "${examples[@]}"; do
  input_dir="output/${example}"

  if [ ! -d "$input_dir" ]; then
    echo "Skipping ${example}: output directory not found"
    continue
  fi

  echo "========================================"
  echo "Rendering: ${example}"
  echo "========================================"

  # Find PVD files
  pvd_files=($input_dir/*.pvd)

  if [ ${#pvd_files[@]} -eq 0 ]; then
    echo "  No PVD files found in $input_dir"
    continue
  fi

  # Use the first PVD file found
  pvd_file="${pvd_files[0]}"
  basename=$(basename "$pvd_file" .pvd)

  echo "  Input: $pvd_file"
  echo "  Rendering video..."

  # Render using ParaView
  $PVPYTHON tools/render_paraview.py --video \
    --input "$input_dir" \
    --output "$input_dir" \
    --prefix "${basename/_rb/}" \
    --prefix "${basename/_md/}" \
    --fps 30 \
    --size 1280 720

  # Convert AVI to MP4 if ffmpeg is available
  if command -v ffmpeg &> /dev/null; then
    for avi in $input_dir/*.avi; do
      if [ -f "$avi" ]; then
        mp4="${avi%.avi}.mp4"
        echo "  Converting to MP4..."
        ffmpeg -y -i "$avi" -c:v libx264 -preset medium -crf 23 -c:a aac "$mp4" > /dev/null 2>&1

        # Copy to docs/assets/videos with example name
        cp "$mp4" "docs/assets/videos/${example}.mp4"
        echo "  ✓ Saved to docs/assets/videos/${example}.mp4"
        rm "$avi"  # Clean up AVI
      fi
    done
  else
    echo "  Warning: ffmpeg not found, keeping AVI format"
    for avi in $input_dir/*.avi; do
      if [ -f "$avi" ]; then
        cp "$avi" "docs/assets/videos/${example}.avi"
      fi
    done
  fi

  echo ""
done

echo "All videos rendered!"
echo "Videos are in docs/assets/videos/"
ls -lh docs/assets/videos/
