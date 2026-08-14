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
