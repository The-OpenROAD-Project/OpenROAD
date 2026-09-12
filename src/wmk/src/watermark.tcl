# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

namespace eval wmk {
# Embedding and verification must derive the same marked population by default.
variable default_routing_fraction 0.05
}

# Read a key file written by generate_watermark_key -file: one "name value"
# pair per line.
proc wmk::read_key_file { path } {
  if { [catch { open $path r } fh] } {
    utl::error WMK 130 "Cannot read key file $path: $fh"
  }
  try {
    set text [read $fh]
  } finally {
    close $fh
  }
  set result [dict create]
  foreach line [split $text \n] {
    set line [string trim $line]
    if { $line eq "" } {
      continue
    }
    if { ![regexp {^(\S+)\s+(\S+)$} $line unused name value] } {
      utl::error WMK 131 "Cannot parse key file $path: '$line'."
    }
    dict set result $name $value
  }
  return $result
}

# The stage key held in a key file: the stored one, or derived from the secret
# key and the public parameters when only those are present.
proc wmk::stage_key_from_file { path stage } {
  set material [wmk::read_key_file $path]
  if { [dict exists $material $stage] } {
    return [dict get $material $stage]
  }
  foreach name { key_hex design_id nonce_hex } {
    if { ![dict exists $material $name] } {
      utl::error WMK 132 "Key file $path has neither a $stage key nor the\
                          key_hex, design_id and nonce_hex to derive it from."
    }
  }
  set derived [wmk::derive_stage_key_cmd [dict get $material key_hex] \
    [dict get $material design_id] [dict get $material nonce_hex] $stage]
  if { $derived eq "" } {
    utl::error WMK 133 "Key file $path holds an invalid key_hex or nonce_hex."
  }
  return $derived
}

proc wmk::check_key_hex { what key } {
  if { ![regexp {^[0-9a-fA-F]{64}$} $key] } {
    utl::error WMK 134 "$what must be a 64-character hex string (32 bytes)."
  }
  return $key
}

# The stage key a command was given: through -key_hex, or read from the key
# file named by -key_file.  Exactly one of the two.
proc wmk::stage_key { command stage keys_name } {
  upvar 1 $keys_name keys
  if { [info exists keys(-key_hex)] && [info exists keys(-key_file)] } {
    utl::error WMK 135 "$command takes -key_hex or -key_file, not both."
  }
  if { [info exists keys(-key_hex)] } {
    return [wmk::check_key_hex "The -key_hex argument" $keys(-key_hex)]
  }
  if { [info exists keys(-key_file)] } {
    return [wmk::stage_key_from_file $keys(-key_file) $stage]
  }
  utl::error WMK 136 "$command requires -key_hex or -key_file."
}

sta::define_cmd_args "generate_watermark_key" {-design_id design_id \
                                               [-file file] \
                                               [-key_hex key_hex] \
                                               [-nonce_hex nonce_hex] \
                                               [-public_file public_file]}

# Draw a watermark secret key and derive the three stage keys from it,
# returning them as a dictionary with keys key_hex, nonce_hex, design_id,
# placement, cts and routing.
#
# The secret key and the nonce are drawn from the system's random source unless
# given, so the same secret key can be reused across designs and revisions: the
# design identifier and the nonce are what make the derived keys differ.  Both
# are needed again at verification time, so record them -- the nonce and the
# identifier are public, the secret key is not.
#
# Nothing is written to the log.  With -file the secret key and the stage keys
# are written to that path with owner-only permissions instead, and left out of
# the returned dictionary: an interactive session echoes every result, and a
# key that was meant for a file should not also end up on the screen.  The
# keyed commands read the file back through -key_file.
#
# -public_file writes only the design identifier and the nonce, at ordinary
# permissions.  Both are needed again to derive the stage keys, and neither is
# secret; without a file of their own the only record of them is the one that
# also holds the secret key, which an owner cannot pass on.
proc generate_watermark_key { args } {
  sta::parse_key_args "generate_watermark_key" args \
    keys {-design_id -nonce_hex -key_hex -file -public_file} flags {}

  if { ![info exists keys(-design_id)] } {
    utl::error WMK 90 "The -design_id argument is required."
  }
  set design_id $keys(-design_id)

  if { [info exists keys(-key_hex)] } {
    set key_hex [wmk::check_key_hex "The -key_hex argument" $keys(-key_hex)]
  } else {
    set key_hex [wmk::random_hex_cmd 32]
    if { $key_hex eq "" } {
      utl::error WMK 92 "Could not read the system random source; refusing to\
                         invent a key."
    }
  }

  if { [info exists keys(-nonce_hex)] } {
    set nonce_hex $keys(-nonce_hex)
    if { [string length $nonce_hex] % 2 != 0 } {
      utl::error WMK 93 "The -nonce_hex argument must be an even-length hex\
                         string."
    }
  } else {
    set nonce_hex [wmk::random_hex_cmd 16]
    if { $nonce_hex eq "" } {
      utl::error WMK 99 "Could not read the system random source; refusing to\
                         invent a nonce."
    }
  }

  set result [dict create key_hex $key_hex nonce_hex $nonce_hex \
    design_id $design_id]
  foreach stage { placement cts routing } {
    set derived [wmk::derive_stage_key_cmd $key_hex $design_id $nonce_hex $stage]
    if { $derived eq "" } {
      utl::error WMK 94 "Could not derive the $stage key; check -key_hex and\
                         -nonce_hex."
    }
    dict set result $stage $derived
  }

  set paths {}
  foreach option { -file -public_file } {
    if { [info exists keys($option)] } {
      dict set paths $option [file normalize $keys($option)]
    }
  }
  wmk::write_key_files $result $paths
  if { [dict exists $paths -file] } {
    return [dict create design_id $design_id nonce_hex $nonce_hex]
  }
  return $result
}

# Validate both destinations before opening either. In particular, a failed
# permission check must not truncate the only saved copy of an owner's key.
proc wmk::check_key_paths { paths } {
  dict for { option path } $paths {
    if { [file exists $path] && ![file isfile $path] } {
      utl::error WMK 115 "$path is not a regular file."
    }
  }
  if { [dict exists $paths -file] && [dict exists $paths -public_file] } {
    set private [dict get $paths -file]
    set public [dict get $paths -public_file]
    set same [expr { $private eq $public }]
    if { [file exists $private] && [file exists $public] } {
      file stat $private private_stat
      file stat $public public_stat
      set same [expr {
        $same || ($private_stat(dev) == $public_stat(dev)
          && $private_stat(ino) == $public_stat(ino))
      }]
    }
    if { $same } {
      utl::error WMK 116 "The private and public key files must be different files."
    }
  }
  if { [dict exists $paths -file] } {
    set path [dict get $paths -file]
    if { [file exists $path] } {
      wmk::check_key_permissions $path
    }
  }
}

proc wmk::check_key_permissions { path } {
  if { [catch { file attributes $path -permissions } mode] } {
    utl::error WMK 104 "Could not confirm owner-only permissions for $path: $mode"
  }
  if { $mode & 0o077 } {
    utl::error WMK 105 "$path is readable by others (mode [format 0%o $mode]);\
                        refusing to write the secret key to it."
  }
}

# Stage in the destination directory so publication is an atomic rename.
# Exclusive creation prevents accidentally opening someone else's temporary
# file; private material is owner-only from the moment the file is created.
proc wmk::stage_key_file { path names result private } {
  set suffix [wmk::random_hex_cmd 16]
  if { $suffix eq "" } {
    utl::error WMK 117 "Could not obtain a temporary key-file name."
  }
  set temporary "$path.wmk-$suffix.tmp"
  set mode [expr { $private ? 0o600 : 0o666 }]
  set fh [open $temporary {WRONLY CREAT EXCL} $mode]
  try {
    if { $private } {
      wmk::check_key_permissions $temporary
    }
    foreach name $names {
      puts $fh "$name [dict get $result $name]"
    }
    close $fh
  } on error { message options } {
    catch { close $fh }
    file delete $temporary
    return -options $options $message
  }
  return $temporary
}

proc wmk::write_key_files { result paths } {
  wmk::check_key_paths $paths
  set staged {}
  try {
    dict for { option path } $paths {
      set private [expr { $option eq "-file" }]
      set names { design_id nonce_hex }
      if { $private } {
        lappend names key_hex placement cts routing
      }
      dict set staged $path [wmk::stage_key_file $path $names $result $private]
    }
    # Both files have been written and closed successfully before replacing
    # either destination. A validation or write error preserves existing files.
    dict for { path temporary } $staged {
      file rename -force $temporary $path
    }
  } finally {
    dict for { path temporary } $staged {
      file delete -force $temporary
    }
  }
  if { [dict exists $paths -file] } {
    utl::info WMK 95 "Wrote the watermark key material to [dict get $paths -file]."
  }
  if { [dict exists $paths -public_file] } {
    utl::info WMK 102 "Wrote the public watermark parameters to [dict get $paths -public_file]."
  }
}

sta::define_cmd_args "derive_watermark_key" {-stage stage \
                                             [-design_id design_id] \
                                             [-key_file key_file] \
                                             [-key_hex key_hex] \
                                             [-nonce_hex nonce_hex]}

# Re-derive one stage key from the secret key, the design identifier and the
# nonce, returning it as a 64-character hex string.  Verification needs the
# same stage keys embedding used, and this is how to get them back without
# storing them.  With -key_file the three inputs are read from a key file
# written by generate_watermark_key.
proc derive_watermark_key { args } {
  sta::parse_key_args "derive_watermark_key" args \
    keys {-key_hex -design_id -nonce_hex -stage -key_file} flags {}

  if { ![info exists keys(-stage)] } {
    utl::error WMK 96 "The -stage argument is required."
  }
  if { [lsearch -exact { placement cts routing } $keys(-stage)] < 0 } {
    utl::error WMK 97 "The -stage argument must be placement, cts or routing."
  }
  if { [info exists keys(-key_file)] } {
    foreach option { -key_hex -design_id -nonce_hex } {
      if { [info exists keys($option)] } {
        utl::error WMK 137 "-key_file replaces $option; give one or the other."
      }
    }
    return [wmk::stage_key_from_file $keys(-key_file) $keys(-stage)]
  }
  foreach required { -key_hex -design_id -nonce_hex } {
    if { ![info exists keys($required)] } {
      utl::error WMK 138 "The $required argument is required without -key_file."
    }
  }
  set derived [wmk::derive_stage_key_cmd $keys(-key_hex) $keys(-design_id) \
    $keys(-nonce_hex) $keys(-stage)]
  if { $derived eq "" } {
    utl::error WMK 98 "Could not derive the key; -key_hex must be 64 hex chars\
                       and -nonce_hex an even-length hex string."
  }
  return $derived
}

sta::define_cmd_args "set_routing_watermark" {[-fraction fraction] \
                                              [-key_file key_file] \
                                              [-key_hex key_hex]}

# Tag a keyed subset of signal nets as watermark nets, returning the number
# tagged.  Selection is keyed unconditionally: a net is selected when the
# first four bytes of HMAC-SHA256(key, "net\0" || name), read as a
# little-endian integer over 2^32, fall below the fraction.  Any previous
# tags are cleared first, so repeated calls are idempotent.
proc set_routing_watermark { args } {
  sta::parse_key_args "set_routing_watermark" args \
    keys {-key_hex -key_file -fraction} flags {}

  set fraction $wmk::default_routing_fraction
  if { [info exists keys(-fraction)] } {
    set fraction $keys(-fraction)
  }
  if { ![string is double -strict $fraction] || !($fraction > 0.0 && $fraction <= 1.0) } {
    utl::error WMK 21 "The -fraction argument must be in (0, 1]."
  }

  set key [wmk::stage_key set_routing_watermark routing keys]
  set rc [wmk::set_routing_watermark_cmd $key $fraction]
  if { $rc < 0 } {
    utl::error WMK 25 "Failed to parse the routing key: must be 64 hex chars."
  }
  return $rc
}

sta::define_cmd_args "place_watermark" {-claims_file file \
                                       [-grid_nx n] \
                                       [-grid_ny n] \
                                       [-guard_degrade_ns ns] \
                                       [-hpwl_eps_um eps] \
                                       [-key_file key_file] \
                                       [-key_hex key_hex] \
                                       [-max_disp_um disp] \
                                       [-min_pairs_total n] \
                                       [-pair_dist_um dist] \
                                       [-pairs_per_tile n] \
                                       [-slack_threshold_ns slack]}

# Put a keyed subset of same-row, same-width cell pairs into a keyed
# left-to-right order, writing all selected pairs to -claims_file. Run after
# detailed placement; the design is re-legalized afterwards. Pairs whose edits
# are rejected or disturbed by legalization remain claimed for verification.
proc place_watermark { args } {
  sta::parse_key_args "place_watermark" args \
    keys {-key_hex -key_file -claims_file -grid_nx -grid_ny -pair_dist_um \
          -pairs_per_tile -slack_threshold_ns -hpwl_eps_um -max_disp_um \
          -min_pairs_total -guard_degrade_ns} \
    flags {}

  if { ![info exists keys(-claims_file)] } {
    utl::error WMK 61 "The -claims_file argument is required."
  }
  set key [wmk::stage_key place_watermark placement keys]

  set grid_nx 8
  set grid_ny 8
  set pair_dist 1.0
  set per_tile 4
  set slack 0.20
  set hpwl_eps 0.05
  set max_disp 5
  set min_pairs 64
  set guard_degrade 0.02
  if { [info exists keys(-grid_nx)] } { set grid_nx $keys(-grid_nx) }
  if { [info exists keys(-grid_ny)] } { set grid_ny $keys(-grid_ny) }
  if { [info exists keys(-pair_dist_um)] } { set pair_dist $keys(-pair_dist_um) }
  if { [info exists keys(-pairs_per_tile)] } { set per_tile $keys(-pairs_per_tile) }
  if { [info exists keys(-slack_threshold_ns)] } { set slack $keys(-slack_threshold_ns) }
  if { [info exists keys(-hpwl_eps_um)] } { set hpwl_eps $keys(-hpwl_eps_um) }
  if { [info exists keys(-max_disp_um)] } { set max_disp $keys(-max_disp_um) }
  if { [info exists keys(-min_pairs_total)] } { set min_pairs $keys(-min_pairs_total) }
  if { [info exists keys(-guard_degrade_ns)] } { set guard_degrade $keys(-guard_degrade_ns) }

  set rc [wmk::place_watermark_cmd $key $keys(-claims_file) \
    $grid_nx $grid_ny $pair_dist $per_tile $slack $hpwl_eps $max_disp \
    $min_pairs $guard_degrade]
  if { $rc < 0 } {
    utl::error WMK 63 "Failed to parse the placement key: must be 64 hex chars."
  }
  return $rc
}

sta::define_cmd_args "cts_watermark" {-claims_file file \
                                     [-cap_headroom_frac frac] \
                                     [-key_file key_file] \
                                     [-key_hex key_hex] \
                                     [-num_pairs n] \
                                     [-sibling_dist_um dist] \
                                     [-skew_margin_ns margin] \
                                     [-slack_margin_ns margin] \
                                     [-slew_headroom_frac frac]}

# Set the sequential fanout parity of a keyed subset of leaf clock buffers,
# writing all selected pairs to -claims_file. Run after clock tree synthesis
# and before routing. Sink moves must respect the timing and electrical limits.
# Pairs whose edits are rejected remain claimed for verification.
proc cts_watermark { args } {
  sta::parse_key_args "cts_watermark" args \
    keys {-key_hex -key_file -claims_file -num_pairs -sibling_dist_um \
          -skew_margin_ns -slack_margin_ns -slew_headroom_frac \
          -cap_headroom_frac} \
    flags {}

  if { ![info exists keys(-claims_file)] } {
    utl::error WMK 65 "The -claims_file argument is required."
  }
  set key [wmk::stage_key cts_watermark cts keys]

  set num_pairs 32
  set dist 20.0
  set margin 0.020
  set slack_margin 0.020
  set slew_frac 0.20
  set cap_frac 0.20
  if { [info exists keys(-num_pairs)] } { set num_pairs $keys(-num_pairs) }
  if { [info exists keys(-sibling_dist_um)] } { set dist $keys(-sibling_dist_um) }
  if { [info exists keys(-skew_margin_ns)] } { set margin $keys(-skew_margin_ns) }
  if { [info exists keys(-slack_margin_ns)] } { set slack_margin $keys(-slack_margin_ns) }
  if { [info exists keys(-slew_headroom_frac)] } { set slew_frac $keys(-slew_headroom_frac) }
  if { [info exists keys(-cap_headroom_frac)] } { set cap_frac $keys(-cap_headroom_frac) }

  set rc [wmk::cts_watermark_cmd $key $keys(-claims_file) \
    $num_pairs $dist $margin $slack_margin $slew_frac $cap_frac]
  if { $rc < 0 } {
    utl::error WMK 67 "Failed to parse the clock-tree key: must be 64 hex chars."
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
  if { ![string is double -strict $p] || !($p > 0.0 && $p < 1.0) } {
    utl::error WMK 22 "The -p argument must be in (0, 1)."
  }

  wmk::report_routing_watermark_cmd $p
}

sta::define_cmd_args "clear_routing_watermark" {}

# Remove every watermark tag from the current block, returning the number
# cleared.
proc clear_routing_watermark { args } {
  sta::check_argc_eq0 "clear_routing_watermark" $args
  sta::parse_key_args "clear_routing_watermark" args keys {} flags {}
  wmk::clear_routing_watermark_cmd
}

sta::define_cmd_args "verify_watermark" {[-claim_alpha alpha] \
                                         [-cts_claims file] \
                                         [-cts_key_hex key_hex] \
                                         [-key_file key_file] \
                                         [-min_stages n] \
                                         [-placement_claims file] \
                                         [-placement_key_hex key_hex] \
                                         [-routing] \
                                         [-routing_alpha alpha] \
                                         [-routing_fraction fraction] \
                                         [-routing_key_hex key_hex] \
                                         [-routing_permutations n] \
                                         [-tau tau]}

# Check the committed placement, CTS and routing watermarks against the loaded
# design.
#
# Every stage is keyed.  A placement or CTS claim file names the marked
# objects; which of each pair is the target and the value it carries are
# derived again from the stage key, and a file whose records disagree with the
# key is refused rather than scored.  Placement and CTS are then decided by the
# extraction rate, the fraction of claims that still hold, against the
# threshold tau, and a binomial chance probability based on the held/checked
# counts against claim_alpha. An exact match is not expected: routing and
# filling legitimately disturb a few marked objects.
#
# Routing has no claims to count -- the marked set is recovered from the key
# alone -- so it is decided by how improbable the marked nets' wrong-way
# wirelength is under a random choice of marked set, against the threshold
# alpha.
#
# Stage keys are given as -placement_key_hex, -cts_key_hex and
# -routing_key_hex, or read from a key file with -key_file.  The routing stage
# is checked when -routing_key_hex is given, or with -routing when the key
# comes from the file.
#
# Ownership is granted when at least -min_stages of the checked stages pass,
# two by default.  Requiring every stage would let one stage with no capacity
# sink a claim the other two prove: a design whose clock tree cannot absorb a
# single moved sink is not thereby unowned.  Requiring only one would accept on
# a single stage's evidence, which is a weaker claim than the scheme intends.
#
# Returns 1 when the design carries the watermark, otherwise 0.
proc verify_watermark { args } {
  sta::parse_key_args "verify_watermark" args \
    keys {-placement_claims -placement_key_hex -cts_claims -cts_key_hex \
          -routing_key_hex -routing_fraction -routing_alpha \
          -routing_permutations -tau -min_stages -claim_alpha -key_file} \
    flags {-routing}

  set tau 0.75
  if { [info exists keys(-tau)] } {
    set tau $keys(-tau)
  }
  if { ![string is double -strict $tau] || !($tau >= 0.0 && $tau <= 1.0) } {
    utl::error WMK 40 "The -tau argument must be in \[0, 1\]."
  }
  set check_routing [expr { [info exists keys(-routing_key_hex)] || [info exists flags(-routing)] }]
  if {
    ![info exists keys(-placement_claims)] && ![info exists keys(-cts_claims)]
    && !$check_routing
  } {
    utl::error WMK 41 "At least one of -placement_claims, -cts_claims,\
                       -routing_key_hex or -routing is required."
  }
  if { [info exists flags(-routing)] && ![info exists keys(-key_file)] } {
    utl::error WMK 139 "-routing takes the routing key from -key_file; give\
                        -key_file or -routing_key_hex."
  }

  set claim_alpha 1e-4
  if { [info exists keys(-claim_alpha)] } {
    set claim_alpha $keys(-claim_alpha)
  }
  if {
    ![string is double -strict $claim_alpha] || !($claim_alpha > 0.0 && $claim_alpha < 1.0)
  } {
    utl::error WMK 119 "The -claim_alpha argument must be in (0, 1)."
  }

  set min_stages 2
  if { [info exists keys(-min_stages)] } {
    set min_stages $keys(-min_stages)
  }
  if { ![string is integer -strict $min_stages] || $min_stages < 1 || $min_stages > 3 } {
    utl::error WMK 88 "The -min_stages argument must be 1, 2 or 3."
  }

  set passed 0
  set checked 0
  # Routing is a population statistic rather than a set of claims.  The sign of
  # T_R is not evidence on its own -- on an unwatermarked design it is a coin
  # flip -- so the stage is judged on the p-value instead.
  if { $check_routing } {
    set routing_key [wmk::verify_stage_key routing keys]
    set frac $wmk::default_routing_fraction
    if { [info exists keys(-routing_fraction)] } {
      set frac $keys(-routing_fraction)
    }
    set alpha 1e-4
    if { [info exists keys(-routing_alpha)] } {
      set alpha $keys(-routing_alpha)
    }
    if { ![string is double -strict $alpha] || !($alpha > 0.0 && $alpha < 1.0) } {
      utl::error WMK 48 "The -routing_alpha argument must be in (0, 1)."
    }
    set draws 100000
    if { [info exists keys(-routing_permutations)] } {
      set draws $keys(-routing_permutations)
    }
    if { ![string is integer -strict $draws] || $draws < 1 || $draws > 2147483647 } {
      utl::error WMK 49 "The -routing_permutations argument must be positive."
    }
    set p_r [wmk::verify_routing_watermark_cmd $routing_key $frac $draws]
    if { $p_r == -2.0 } {
      # The technology has no wrong-way routing to measure, so the stage is not
      # applicable rather than failed, and it does not count against the tally.
      utl::warn WMK 87 "No routing carrier in this technology; stage skipped."
    } elseif { $p_r < 0.0 } {
      utl::error WMK 23 "Failed to parse the routing key: must be 64 hex chars."
    } else {
      incr checked
      if { $p_r > $alpha } {
        utl::warn WMK 47 \
          "Routing shows no watermark: p = [format %.2e $p_r] >\
           [format %.2e $alpha]."
      } else {
        incr passed
      }
    }
  }

  foreach { key stage cmd } {
    -placement_claims placement wmk::verify_placement_claims_cmd
    -cts_claims       cts       wmk::verify_cts_claims_cmd
  } {
    if { ![info exists keys($key)] } {
      continue
    }
    set stage_key [wmk::verify_stage_key $stage keys]
    lassign [$cmd $stage_key $keys($key)] count held probability
    if { $count == 0 } {
      utl::warn WMK 42 "No checkable $stage claims; stage skipped."
      continue
    }
    incr checked
    set rate [expr { double($held) / $count }]
    if { $rate < $tau } {
      utl::warn WMK 43 \
        "Stage $stage below threshold: [format %.4f $rate] < [format %.4f $tau]."
    } elseif { $probability > $claim_alpha } {
      utl::warn WMK 120 \
        "Insufficient $stage evidence: $held of $count claims hold;\
         binomial p = [format %.4g $probability] > [format %.4g $claim_alpha]."
    } else {
      incr passed
    }
  }

  if { $checked == 0 } {
    utl::warn WMK 44 "No stage had checkable claims; no verdict."
    return 0
  }
  if { $checked < $min_stages } {
    utl::warn WMK 89 "Only $checked stage(s) could be checked, fewer than the\
                      $min_stages required; pass -min_stages to decide on\
                      fewer."
  }
  if { $passed >= $min_stages } {
    utl::info WMK 45 \
      "Ownership evidence holds in $passed of $checked stage(s) checked."
    return 1
  }
  utl::info WMK 46 \
    "Ownership evidence does not hold: $passed of $checked stage(s) passed,\
     $min_stages required."
  return 0
}

# The key verify_watermark checks one stage with: the explicit
# -<stage>_key_hex, or the stage's key from -key_file.
proc wmk::verify_stage_key { stage keys_name } {
  upvar 1 $keys_name keys
  set option -${stage}_key_hex
  if { [info exists keys($option)] } {
    return [wmk::check_key_hex "The $option argument" $keys($option)]
  }
  if { [info exists keys(-key_file)] } {
    return [wmk::stage_key_from_file $keys(-key_file) $stage]
  }
  utl::error WMK 140 "Checking the $stage stage requires $option or -key_file."
}
