#!/usr/bin/env bash
#
# Check if contents of $1 is equal to "OK"
set -e
if [ "$(cat "$1")" == "OK" ]; then
  exit 0
fi
exit 1
