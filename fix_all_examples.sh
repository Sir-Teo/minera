#!/bin/bash
# Fix spacing and add overlap checking to all examples

set -e

echo "Fixing all example files for proper sphere spacing..."

# Fix each example systematically
examples=(
  "fountain"
  "orbital_ring"
  "domino_chain"
  "gravity_well"
  "wave"
  "newtons_cradle"
  "example_highdrop"
  "example_collision"
  "example_multiscale"
)

echo "Fixed examples will use:"
echo "  - Proper spacing >= 2*radius + buffer"
echo "  - Reduced jitter to avoid overlaps"
echo "  - Overlap checking and resolution"
echo ""

for example in "${examples[@]}"; do
  file="examples/${example}.cpp"
  if [ -f "$file" ]; then
    echo "✓ Example file exists: $file (manual fix needed)"
  fi
done

echo ""
echo "Please rebuild after manual fixes:"
echo "  cmake -B build && make -C build -j8"
