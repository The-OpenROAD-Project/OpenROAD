#!/usr/bin/env bash
#
# Check if contents of $1 is equal to "OK"
[ "$(cat "$1")" == "OK" ]
