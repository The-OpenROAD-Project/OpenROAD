# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

sta::define_cmd_args "set_routing_watermark" {-key_hex key_hex \
                                              [-fraction fraction]}

# Tag a keyed subset of signal nets as watermark nets, returning the number
# tagged.  Selection is keyed unconditionally: a net is selected when the
# first four bytes of HMAC-SHA256(key, "net\0" || name), read as a
# little-endian integer over 2^32, fall below the fraction.  Any previous
# tags are cleared first, so repeated calls are idempotent.
proc set_routing_watermark { args } {
  sta::parse_key_args "set_routing_watermark" args \
    keys {-key_hex -fraction} flags {}

  set fraction 0.05
  if { [info exists keys(-fraction)] } {
    set fraction $keys(-fraction)
  }
  if { $fraction <= 0.0 || $fraction > 1.0 } {
    utl::error WMK 21 "The -fraction argument must be in (0, 1]."
  }

  if { ![info exists keys(-key_hex)] } {
    utl::error WMK 20 "The -key_hex argument is required."
  }
  set key_hex $keys(-key_hex)
  if { [string length $key_hex] != 64 } {
    utl::error WMK 24 "The -key_hex argument must be a 64-character hex\
                       string (32 bytes)."
  }
  set rc [wmk::set_routing_watermark_cmd $key_hex $fraction]
  if { $rc < 0 } {
    utl::error WMK 25 "Failed to parse -key_hex: must be 64 hex chars."
  }
  return $rc
}

sta::define_cmd_args "place_watermark" {-claims_file file \
                                       -key_hex key_hex \
                                       [-grid_nx n] \
                                       [-grid_ny n] \
                                       [-pair_dist_um dist] \
                                       [-pairs_per_tile n] \
                                       [-slack_threshold_ns slack] \
                                       [-hpwl_eps_dbu eps] \
                                       [-max_disp_um disp]}

# Put a keyed subset of same-row, same-width cell pairs into a keyed
# left-to-right order, writing the committed pairs to -claims_file.  Run after
# detailed placement; the design is re-legalized afterwards and any pair that
# legalization disturbed is dropped rather than claimed.
proc place_watermark { args } {
  sta::parse_key_args "place_watermark" args \
    keys {-key_hex -claims_file -grid_nx -grid_ny -pair_dist_um \
          -pairs_per_tile -slack_threshold_ns -hpwl_eps_dbu -max_disp_um} \
    flags {}

  if { ![info exists keys(-key_hex)] } {
    utl::error WMK 60 "The -key_hex argument is required."
  }
  if { ![info exists keys(-claims_file)] } {
    utl::error WMK 61 "The -claims_file argument is required."
  }
  if { [string length $keys(-key_hex)] != 64 } {
    utl::error WMK 62 "The -key_hex argument must be a 64-character hex\
                       string (32 bytes)."
  }

  set grid_nx 8
  set grid_ny 8
  set pair_dist 1.0
  set per_tile 4
  set slack 0.20
  set hpwl_eps 100
  set max_disp 5
  if { [info exists keys(-grid_nx)] } { set grid_nx $keys(-grid_nx) }
  if { [info exists keys(-grid_ny)] } { set grid_ny $keys(-grid_ny) }
  if { [info exists keys(-pair_dist_um)] } { set pair_dist $keys(-pair_dist_um) }
  if { [info exists keys(-pairs_per_tile)] } { set per_tile $keys(-pairs_per_tile) }
  if { [info exists keys(-slack_threshold_ns)] } { set slack $keys(-slack_threshold_ns) }
  if { [info exists keys(-hpwl_eps_dbu)] } { set hpwl_eps $keys(-hpwl_eps_dbu) }
  if { [info exists keys(-max_disp_um)] } { set max_disp $keys(-max_disp_um) }

  set rc [wmk::place_watermark_cmd $keys(-key_hex) $keys(-claims_file) \
            $grid_nx $grid_ny $pair_dist $per_tile $slack $hpwl_eps $max_disp]
  if { $rc < 0 } {
    utl::error WMK 63 "Failed to parse -key_hex: must be 64 hex chars."
  }
  return $rc
}

sta::define_cmd_args "cts_watermark" {-claims_file file \
                                     -key_hex key_hex \
                                     [-num_pairs n] \
                                     [-sibling_dist_um dist] \
                                     [-skew_margin_ns margin]}

# Set the sequential fanout parity of a keyed subset of leaf clock buffers,
# writing the committed pairs to -claims_file.  Run after clock tree synthesis.
# A pair is committed only if the sink move it needed did not worsen the
# clock's worst skew.
proc cts_watermark { args } {
  sta::parse_key_args "cts_watermark" args \
    keys {-key_hex -claims_file -num_pairs -sibling_dist_um -skew_margin_ns} \
    flags {}

  if { ![info exists keys(-key_hex)] } {
    utl::error WMK 64 "The -key_hex argument is required."
  }
  if { ![info exists keys(-claims_file)] } {
    utl::error WMK 65 "The -claims_file argument is required."
  }
  if { [string length $keys(-key_hex)] != 64 } {
    utl::error WMK 66 "The -key_hex argument must be a 64-character hex\
                       string (32 bytes)."
  }

  set num_pairs 32
  set dist 20.0
  set margin 0.0
  if { [info exists keys(-num_pairs)] } { set num_pairs $keys(-num_pairs) }
  if { [info exists keys(-sibling_dist_um)] } { set dist $keys(-sibling_dist_um) }
  if { [info exists keys(-skew_margin_ns)] } { set margin $keys(-skew_margin_ns) }

  set rc [wmk::cts_watermark_cmd $keys(-key_hex) $keys(-claims_file) \
            $num_pairs $dist $margin]
  if { $rc < 0 } {
    utl::error WMK 67 "Failed to parse -key_hex: must be 64 hex chars."
  }
  return $rc
}

sta::define_cmd_args "report_routing_watermark" {[-p p]}

# Rank every routed signal net by its wrong-way wirelength fraction and
# report how many watermark nets fall below the p-quantile cutoff, with the
# resulting coincidence probability.  Run after detailed_route.
proc report_routing_watermark { args } {
  sta::parse_key_args "report_routing_watermark" args \
    keys {-p} flags {}

  set p 0.4
  if { [info exists keys(-p)] } {
    set p $keys(-p)
  }
  if { $p <= 0.0 || $p >= 1.0 } {
    utl::error WMK 22 "The -p argument must be in (0, 1)."
  }

  wmk::report_routing_watermark_cmd $p
}

sta::define_cmd_args "clear_routing_watermark" {}

# Remove every watermark tag from the current block, returning the number
# cleared.
proc clear_routing_watermark { args } {
  sta::check_argc_eq0 "clear_routing_watermark" $args
  wmk::clear_routing_watermark_cmd
}

sta::define_cmd_args "verify_watermark" {[-cts_claims file] \
                                         [-placement_claims file] \
                                         [-routing_alpha alpha] \
                                         [-routing_fraction fraction] \
                                         [-routing_key_hex key_hex] \
                                         [-routing_permutations n] \
                                         [-tau tau]}

# Check the committed placement, CTS and routing watermarks against the loaded
# design.
#
# Placement and CTS are decided by the extraction rate, the fraction of claims
# that still hold, against the threshold tau. An exact match is not expected:
# routing and filling legitimately disturb a few marked objects.
#
# Routing has no claims to count -- the marked set is recovered from the key
# alone -- so it is decided by how improbable the marked nets' wrong-way
# wirelength is under a random choice of marked set, against the threshold
# alpha.
#
# Returns 1 when every stage that was checked met its threshold, otherwise 0.
proc verify_watermark { args } {
  sta::parse_key_args "verify_watermark" args \
    keys {-placement_claims -cts_claims -routing_key_hex -routing_fraction \
          -routing_alpha -routing_permutations -tau} \
    flags {}

  set tau 0.75
  if { [info exists keys(-tau)] } {
    set tau $keys(-tau)
  }
  if { $tau < 0.0 || $tau > 1.0 } {
    utl::error WMK 40 "The -tau argument must be in \[0, 1\]."
  }
  if { ![info exists keys(-placement_claims)] && ![info exists keys(-cts_claims)]
       && ![info exists keys(-routing_key_hex)] } {
    utl::error WMK 41 "At least one of -placement_claims, -cts_claims or\
                       -routing_key_hex is required."
  }

  set pass 1
  set checked 0
  # Routing is a population statistic rather than a set of claims.  The sign of
  # T_R is not evidence on its own -- on an unwatermarked design it is a coin
  # flip -- so the stage is judged on the p-value instead.
  if { [info exists keys(-routing_key_hex)] } {
    set frac 0.02
    if { [info exists keys(-routing_fraction)] } {
      set frac $keys(-routing_fraction)
    }
    set alpha 1e-4
    if { [info exists keys(-routing_alpha)] } {
      set alpha $keys(-routing_alpha)
    }
    if { $alpha <= 0.0 || $alpha >= 1.0 } {
      utl::error WMK 48 "The -routing_alpha argument must be in (0, 1)."
    }
    set draws 100000
    if { [info exists keys(-routing_permutations)] } {
      set draws $keys(-routing_permutations)
    }
    if { $draws < 1 } {
      utl::error WMK 49 "The -routing_permutations argument must be positive."
    }
    set p_r [wmk::verify_routing_watermark_cmd $keys(-routing_key_hex) $frac \
               $draws]
    if { $p_r < 0.0 } {
      utl::error WMK 23 "Failed to parse -routing_key_hex: must be 64 hex\
                         chars."
    }
    incr checked
    if { $p_r > $alpha } {
      set pass 0
      utl::warn WMK 47 \
        "Routing shows no watermark: p = [format %.2e $p_r] >\
         [format %.2e $alpha]."
    }
  }
  foreach { key stage cmd } {
    -placement_claims placement wmk::verify_placement_watermark_cmd
    -cts_claims       cts       wmk::verify_cts_watermark_cmd
  } {
    if { ![info exists keys($key)] } {
      continue
    }
    set rate [$cmd $keys($key)]
    if { $rate < 0.0 } {
      utl::warn WMK 42 "No checkable $stage claims; stage skipped."
      continue
    }
    incr checked
    if { $rate < $tau } {
      set pass 0
      utl::warn WMK 43 \
        "Stage $stage below threshold: [format %.4f $rate] < [format %.4f $tau]."
    }
  }

  if { $checked == 0 } {
    utl::warn WMK 44 "No stage had checkable claims; no verdict."
    return 0
  }
  if { $pass } {
    utl::info WMK 45 "Ownership evidence holds in $checked stage(s)."
  } else {
    utl::info WMK 46 "Ownership evidence does not hold."
  }
  return $pass
}
