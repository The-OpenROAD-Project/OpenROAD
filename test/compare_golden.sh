#!/usr/bin/env bash
# Compare bazel test.log files against their golden .ok files.
#
# Usage: compare_golden.sh <list_file> [out_dir] [openroad_root]
#   list_file      one bazel test.log path per line (leading/trailing spaces ok)
#   out_dir        where diffs go (default: ./golden_cmp)
#   openroad_root  source tree with the .ok files (default: ~/OpenROAD)
#
# Mapping example:
#   .../testlogs/src/rsz/test/clone_flat-tcl_test/test.log
#   -> <openroad_root>/src/rsz/test/clone_flat.ok
#
# Output:
#   <out_dir>/<pkg_with_underscores>__<test>.diff   unified diff per test (only if different)
#   <out_dir>/summary.txt                            PASS/DIFF/MISSING per test

set -u

list_file=${1:?usage: $0 <list_file> [out_dir] [openroad_root]}
out_dir=${2:-./golden_cmp}
or_root=${3:-$HOME/OpenROAD}

mkdir -p "$out_dir"
summary="$out_dir/summary.txt"
: > "$summary"

n_pass=0 n_diff=0 n_miss=0

# Drop the bazel wrapper header that precedes the actual test output:
#   exec ${PAGER:-/usr/bin/less} "$0" || exit 1
#   Executing tests from //src/rsz/test:clone_flat-tcl_test
#   -----------------------------------------------------------------------------
strip_bazel_header() {
  awk 'hdr && (/^exec \$\{PAGER/ || /^Executing tests from / || /^-{20,}$/) {next}
       {hdr=0; print}' hdr=1 "$1"
}

while IFS= read -r line || [[ -n $line ]]; do
  # trim whitespace, skip blanks/comments
  log=$(echo "$line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')
  [[ -z $log || $log == \#* ]] && continue

  rel=${log#*/testlogs/}                 # src/rsz/test/clone_flat-tcl_test/test.log
  test_dir=$(dirname "$rel")             # src/rsz/test/clone_flat-tcl_test
  pkg=$(dirname "$test_dir")             # src/rsz/test
  target=$(basename "$test_dir")         # clone_flat-tcl_test
  name=${target%-tcl_test}; name=${name%-py_test}   # clone_flat
  golden="$or_root/$pkg/$name.ok"
  tag="${pkg//\//_}__${name}"

  if [[ ! -f $log ]]; then
    echo "MISSING_LOG  $name  $log" | tee -a "$summary"; ((n_miss++)); continue
  fi
  if [[ ! -f $golden ]]; then
    echo "MISSING_OK   $name  $golden" | tee -a "$summary"; ((n_miss++)); continue
  fi

  diff_file="$out_dir/$tag.diff"
  if diff -u --label "golden: $golden" --label "log: $log" \
       "$golden" <(strip_bazel_header "$log") > "$diff_file"; then
    rm -f "$diff_file"
    echo "PASS         $name" | tee -a "$summary"; ((n_pass++))
  else
    nlines=$(grep -c '^[-+][^-+]' "$diff_file")
    echo "DIFF         $name  ($nlines changed lines)  $diff_file" | tee -a "$summary"
    ((n_diff++))
  fi
done < "$list_file"

echo "----" | tee -a "$summary"
echo "PASS=$n_pass  DIFF=$n_diff  MISSING=$n_miss" | tee -a "$summary"
