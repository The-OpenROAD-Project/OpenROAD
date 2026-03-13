#!/bin/bash
# Benchmark: measure delta sizes between consecutive ORFS stages
# Usage: ./benchmark_delta.sh [OPENROAD_EXE] [STAGE_DIR]
#
# Example:
#   ./benchmark_delta.sh ../../bazel-bin/openroad \
#     ../../bazel-bin/test/orfs/mock-array/results/asap7/MockArray/4x4_base/

set -e

OPENROAD="${1:-openroad}"
STAGE_DIR="${2:-../../bazel-bin/test/orfs/mock-array/results/asap7/MockArray/4x4_base}"
RESULTS_DIR=$(mktemp -d)

# Ordered stages
STAGES=(1_synth 2_floorplan 3_place 4_cts 5_1_grt 5_route 6_final)

# Filter to available stages
AVAILABLE=()
for stage in "${STAGES[@]}"; do
  f="${STAGE_DIR}/${stage}.odb"
  if [ -f "$f" ]; then
    AVAILABLE+=("$stage")
  else
    echo "SKIP: $f not found"
  fi
done

if [ ${#AVAILABLE[@]} -lt 2 ]; then
  echo "ERROR: need at least 2 stage .odb files"
  exit 1
fi

echo ""
echo "=== Delta Size Benchmark (MockArray 4x4, ASAP7) ==="
echo "OpenROAD: $OPENROAD"
echo "Stage dir: $STAGE_DIR"
echo "Results dir: $RESULTS_DIR"
echo ""
printf "%-28s %12s %12s %12s %8s %s\n" \
  "Transition" "Base (KB)" "Full (KB)" "Delta (KB)" "Ratio" "Recommendation"
printf '%0.s-' {1..95}; echo ""

total_full=0
total_delta=0
total_base_for_deltas=0

for ((i=0; i < ${#AVAILABLE[@]} - 1; i++)); do
  base_stage="${AVAILABLE[$i]}"
  next_stage="${AVAILABLE[$((i+1))]}"

  base_file="${STAGE_DIR}/${base_stage}.odb"
  next_file="${STAGE_DIR}/${next_stage}.odb"
  delta_file="${RESULTS_DIR}/delta_${base_stage}_to_${next_stage}.delta"

  base_size=$(stat -c%s "$base_file")
  next_size=$(stat -c%s "$next_file")

  # Compute delta: load next_file, write delta against base_file
  $OPENROAD -no_init -no_splash -exit <<EOF
read_db $next_file
write_db -base $base_file $delta_file
EOF

  delta_size=$(stat -c%s "$delta_file")

  base_kb=$(echo "scale=1; $base_size / 1024" | bc)
  next_kb=$(echo "scale=1; $next_size / 1024" | bc)
  delta_kb=$(echo "scale=1; $delta_size / 1024" | bc)
  ratio=$(echo "scale=1; 100 * $delta_size / $next_size" | bc)

  if [ "$(echo "$delta_size < $next_size / 2" | bc)" -eq 1 ]; then
    rec="delta"
  else
    rec="full .odb"
  fi

  printf "%-28s %12s %12s %12s %7s%% %s\n" \
    "${base_stage} -> ${next_stage}" "$base_kb" "$next_kb" "$delta_kb" "$ratio" "$rec"

  total_full=$((total_full + next_size))
  total_delta=$((total_delta + delta_size))
done

echo ""
total_full_kb=$(echo "scale=1; $total_full / 1024" | bc)
total_delta_kb=$(echo "scale=1; $total_delta / 1024" | bc)
overall_ratio=$(echo "scale=1; 100 * $total_delta / $total_full" | bc)
echo "Total full: ${total_full_kb} KB, Total delta: ${total_delta_kb} KB, Overall ratio: ${overall_ratio}%"
echo ""

# Also measure compressed sizes
echo "=== Compressed Sizes (gzip -9) ==="
echo ""
printf "%-28s %12s %12s %12s %12s\n" \
  "Transition" "Full.gz (KB)" "Delta.gz (KB)" "Savings (KB)" "gz Ratio"
printf '%0.s-' {1..78}; echo ""

for ((i=0; i < ${#AVAILABLE[@]} - 1; i++)); do
  base_stage="${AVAILABLE[$i]}"
  next_stage="${AVAILABLE[$((i+1))]}"

  next_file="${STAGE_DIR}/${next_stage}.odb"
  delta_file="${RESULTS_DIR}/delta_${base_stage}_to_${next_stage}.delta"

  gzip -9 -c "$next_file" > "${RESULTS_DIR}/${next_stage}.odb.gz"
  gzip -9 -c "$delta_file" > "${delta_file}.gz"

  full_gz_size=$(stat -c%s "${RESULTS_DIR}/${next_stage}.odb.gz")
  delta_gz_size=$(stat -c%s "${delta_file}.gz")

  full_gz_kb=$(echo "scale=1; $full_gz_size / 1024" | bc)
  delta_gz_kb=$(echo "scale=1; $delta_gz_size / 1024" | bc)
  savings_kb=$(echo "scale=1; ($full_gz_size - $delta_gz_size) / 1024" | bc)
  gz_ratio=$(echo "scale=1; 100 * $delta_gz_size / $full_gz_size" | bc)

  printf "%-28s %12s %12s %12s %11s%%\n" \
    "${base_stage} -> ${next_stage}" "$full_gz_kb" "$delta_gz_kb" "$savings_kb" "$gz_ratio"
done

echo ""
echo "Benchmark complete."
rm -rf "$RESULTS_DIR"
