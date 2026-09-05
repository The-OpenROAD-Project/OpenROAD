# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

or_integration_tests(
  "wmk"
  TESTS
    bias
    bias_neutral
    place_unmarked
    route_stat
    select
    strength
    verify
    via_layer
  # These run a placer, a clock tree synthesizer or a router, and would
  # otherwise pin those tools' logs into a watermarking golden.  They check
  # their own numbers instead.
  PASSFAIL_TESTS
    cts
    cts_drive
    cts_guard
    keygen
    place
    place_guard
    place_keyed
    python_api
    route_verify
)

add_subdirectory(cpp)
