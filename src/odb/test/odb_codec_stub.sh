#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# A stand-in for a user's ODB_CODEC, for the odb_codec_* tests. encode
# checks that the layout describes the file it was handed and prefixes a
# marker line; decode strips the marker and hands any other file over
# unchanged, as a codec must.
set -e
marker=odb-codec-stub
case "$1" in
encode)
  size=$(wc -c <"$2")
  head -n 1 "$3" | grep -qx 'odb-layout 1'
  tail -n 1 "$3" | grep -qx "size $((size))"
  { echo "$marker"; cat "$2"; } >"$2.stub"
  mv "$2.stub" "$2"
  ;;
decode)
  if [ "$(head -n 1 "$2")" = "$marker" ]; then
    tail -n +2 "$2"
  else
    cat "$2"
  fi
  ;;
*)
  exit 2
  ;;
esac
