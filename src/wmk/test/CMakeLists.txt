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
    claim_publication
    cts_empty_claims
    cts_headroom
    numeric_options
    key_files
    cts_modes
    verify_nul
    strength_limits
    cts_protected
    cts_hierarchy
    cts_claim_names
    place_claim_names
    cts
    cts_drive
    cts_guard
    cts_endpoint_guard
    cts_clock_logic
    cts_liberty
    cts_clock_sets
    cts_skew_domains
    keygen
    place
    place_full_rollback
    place_guard
    place_keyed
    place_unconstrained
    python_api
    route_verify
    routing_calibration
    routing_defaults
    routing_lifecycle
    routing_report
    routing_ties
    verify_malformed
)

add_subdirectory(cpp)
