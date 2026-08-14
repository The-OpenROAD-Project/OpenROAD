# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

sta::define_cmd_args "set_routing_watermark" {[-message message] \
                                              [-key_hex key_hex] \
                                              [-fraction fraction]}

proc set_routing_watermark { args } {
  sta::parse_key_args "set_routing_watermark" args \
    keys {-message -key_hex -fraction} flags {}

  set fraction 0.05
  if { [info exists keys(-fraction)] } {
    set fraction $keys(-fraction)
  }
  if { $fraction <= 0.0 || $fraction > 1.0 } {
    utl::error WMK 21 "The -fraction argument must be in (0, 1]."
  }

  set have_key [info exists keys(-key_hex)]
  set have_msg [info exists keys(-message)]
  if { !$have_key && !$have_msg } {
    utl::error WMK 20 "Either -key_hex or -message is required."
  }
  if { $have_key && $have_msg } {
    utl::error WMK 23 "Pass either -key_hex (Kerckhoffs-compliant) or\
                       -message (legacy), not both."
  }

  if { $have_key } {
    set key_hex $keys(-key_hex)
    if { [string length $key_hex] != 64 } {
      utl::error WMK 24 "The -key_hex argument must be a 64-character hex\
                         string (32 bytes)."
    }
    set rc [wmk::set_routing_watermark_keyed_cmd $key_hex $fraction]
    if { $rc < 0 } {
      utl::error WMK 25 "Failed to parse -key_hex: must be 64 hex chars."
    }
    return $rc
  } else {
    wmk::set_routing_watermark_cmd $keys(-message) $fraction
  }
}

sta::define_cmd_args "report_routing_watermark" {[-p p]}

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

proc clear_routing_watermark { args } {
  wmk::clear_routing_watermark_cmd
}

sta::define_cmd_args "verify_watermark" {[-placement_claims file] \
                                         [-cts_claims file] \
                                         [-tau tau]}

# Check the placement and CTS watermarks against the loaded design.
#
# Ownership is decided by the extraction rate, the fraction of committed claims
# that still hold, against the threshold tau. An exact match is not expected:
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
