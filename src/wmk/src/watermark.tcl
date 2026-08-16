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
                                         [-tau tau]}

# Check the committed placement and CTS watermarks against the loaded design.
#
# Ownership is decided by the extraction rate, the fraction of claims that
# still hold, against the threshold tau. An exact match is not expected:
# routing and filling legitimately disturb a few marked objects.
#
# Returns 1 when every stage that had claims met the threshold, otherwise 0.
proc verify_watermark { args } {
  sta::parse_key_args "verify_watermark" args \
    keys {-placement_claims -cts_claims -tau} flags {}

  set tau 0.75
  if { [info exists keys(-tau)] } {
    set tau $keys(-tau)
  }
  if { $tau < 0.0 || $tau > 1.0 } {
    utl::error WMK 40 "The -tau argument must be in \[0, 1\]."
  }
  if { ![info exists keys(-placement_claims)] && ![info exists keys(-cts_claims)] } {
    utl::error WMK 41 "At least one of -placement_claims or -cts_claims is required."
  }

  set pass 1
  set checked 0
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
