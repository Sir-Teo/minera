#!/bin/bash
# Run all new example simulations

set -e

echo "Running all example simulations..."
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
)

for example in "${examples[@]}"; do
  echo "========================================"
  echo "Running: example_${example}"
  echo "========================================"
  ./build/example_${example}
  echo ""
  echo "Completed: example_${example}"
  echo ""
done

echo "All simulations complete!"
echo "VTK output files are in output/*/ directories"
