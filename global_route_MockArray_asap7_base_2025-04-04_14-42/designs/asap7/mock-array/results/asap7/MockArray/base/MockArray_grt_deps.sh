#!/usr/bin/env bash

set -e

usage() {
  echo "Usage: $1 [ABSOLUTE_PATH]"
  exit 1
}

main() {
  local progname
  local dst
  local config
  local genfiles
  local renames
  local make
  progname=$(basename "$0")

  while [ $# -gt 0 ]; do
    case $1 in
      -h|--help)
        usage "$progname"
      ;;
      -c|--config)
        config="$2"
        shift
        shift
      ;;
      -g|--genfiles)
        genfiles="$2"
        shift
        shift
      ;;
      -r|--renames)
        renames="$2"
        shift
        shift
      ;;
      -m|--make)
        make="$2"
        shift
        shift
      ;;
      *)
        dst="$1"
        shift
        break
      ;;
    esac
  done

  if [ -z "$dst" ]; then
    echo "$progname: must have [ABSOLUTE_PATH]"
    echo "Try '$progname -h' for more information."
    exit 1
  fi

  local canonical
  canonical="$(realpath --canonicalize-missing "$dst")"
  if [ "$dst" != "$canonical" ] && [ "$dst" != "$canonical/" ]; then
    echo "$progname: '$dst' is not an absolute path"
    echo "Try '$progname -h' for more information."
    exit 1
  fi

  mkdir --parents "$dst"
  cp --recursive --parents --target-directory "$dst" -- *

  for file in $genfiles; do
    if [ -L "$dst/$file" ]; then
      unlink "$dst/$file"
    fi
    cp --force --dereference --no-preserve=all --parents --target-directory "$dst" "$file"
  done

  cp --force "$make" "$dst/make"
  cp --force --no-preserve=all "$config" "$dst/config.mk"

  for rename in $renames; do
    IFS=':' read -r from to <<EOF
$rename
EOF
    mkdir --parents "$dst"/"$(dirname "$to")"
    cp --force --dereference --no-preserve=all "$from" "$dst"/"$to"
  done

  if [ "$#" -gt 0 ]; then
    "$dst/make" "$@"
  fi
}

main --genfiles "designs/asap7/mock-array/reports/asap7/MockArray/base/4_cts_final.rpt designs/asap7/mock-array/results/asap7/MockArray/base/4_cts.odb designs/asap7/mock-array/results/asap7/MockArray/base/4_cts.sdc designs/asap7/mock-array/results/asap7/MockArray/base/5_1_grt.short.mk" --renames "" --make "designs/asap7/mock-array/make_MockArray_grt_base_5_1_grt" --config "designs/asap7/mock-array/results/asap7/MockArray/base/5_1_grt.short.mk" "$@"
