#!/bin/bash
# Run a subset of examples quickly for video generation

set -e

echo "Running quick examples for video generation..."
echo ""

# Run only the new examples with shorter durations
examples=(
  "orbital_ring"
  "domino_chain"
  "fountain"
  "vortex"
  "gravity_well"
  "crystal"
  "wave"
)

for example in "${examples[@]}"; do
  echo "Running: example_${example}"
  ./build/example_${example} > /dev/null 2>&1 &
  pid=$!

  # Wait max 60 seconds per example
  timeout=60
  elapsed=0
  while kill -0 $pid 2>/dev/null && [ $elapsed -lt $timeout ]; do
    sleep 5
    elapsed=$((elapsed + 5))
    echo "  ... $elapsed seconds"
  done

  if kill -0 $pid 2>/dev/null; then
    kill $pid 2>/dev/null
    wait $pid 2>/dev/null || true
  fi

  echo "  ✓ Completed example_${example}"
  echo ""
done

echo "All quick examples complete!"
