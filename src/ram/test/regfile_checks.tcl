# Checks of a generated register file by what it computes, not by how it
# is laid out: case analysis on its inputs and on its storage, and
# OpenSTA's constant propagation through its cells. Sourced by the
# generate_regfile tests after helpers.tcl.

proc rf_read_libraries { } {
  read_lef asap7/asap7_tech_1x_201209.lef
  read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
  foreach kind { AO OA SIMPLE } {
    read_liberty asap7/asap7sc7p5t_${kind}_RVT_FF_nldm_211120.lib.gz
  }
  read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
  read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
}

# A spec file: the base spec with `lines` appended, in the results dir.
proc rf_spec { base name lines } {
  set spec [make_result_file $name]
  set out [open $spec w]
  set in [open $base r]
  puts -nonewline $out [read $in]
  close $in
  foreach line $lines {
    puts $out $line
  }
  close $out
  return $spec
}

# The propagated value of a port (by name) or an instance pin.
proc rf_value { name } {
  set port [get_ports -quiet $name]
  if { $port ne "" } {
    return [sta::pin_sim_logic_value [sta::get_port_pin $port]]
  }
  return [sta::pin_sim_logic_value [get_pins $name]]
}

proc rf_drive_bit { name value } {
  set_case_analysis $value [get_ports $name]
}

proc rf_drive { bus width value } {
  for { set i 0 } { $i < $width } { incr i } {
    rf_drive_bit "$bus\[$i\]" [expr { ($value >> $i) & 1 }]
  }
}

# The integer an output bus carries, or X if any bit is not a constant.
proc rf_read { bus width } {
  set v 0
  for { set i 0 } { $i < $width } { incr i } {
    set b [rf_value "$bus\[$i\]"]
    if { $b ne "0" && $b ne "1" } {
      return X
    }
    set v [expr { $v | ($b << $i) }]
  }
  return $v
}

# Word `word` holding `value`: the storage flop of each bit holds the
# inverse on QN, which is where the value is set.
proc rf_store { word bits value } {
  for { set b 0 } { $b < $bits } { incr b } {
    set_case_analysis [expr { 1 - (($value >> $b) & 1) }] \
      [get_pins w${word}_b${b}_ff/QN]
  }
}

# The integer word `word`'s flops would load: their D inputs.
proc rf_next { word bits } {
  set v 0
  for { set b 0 } { $b < $bits } { incr b } {
    set d [rf_value w${word}_b${b}_ff/D]
    if { $d ne "0" && $d ne "1" } {
      return X
    }
    set v [expr { $v | ($d << $b) }]
  }
  return $v
}

# A value per word, distinct across words, for the storage.
proc rf_word_value { word bits } {
  return [expr { ($word * 5 + 3) & ((1 << $bits) - 1) }]
}

# The ports of the generated block missing from a view: the abstract
# (`PIN <name>`) or the timing model (`pin(<name>)`, or a `bus(<base>)`
# for a bus bit).
proc rf_ports_missing_from { file kind } {
  set in [open $file r]
  set text [read $in]
  close $in
  set missing {}
  foreach bterm [[ord::get_db_block] getBTerms] {
    set name [$bterm getName]
    set base [lindex [split $name {[}] 0]
    if { $kind eq "lef" } {
      set found [expr { [string first "PIN $name\n" $text] >= 0 }]
    } else {
      set found [expr {
        [string first "pin($name)" $text] >= 0
        || [string first "bus($base)" $text] >= 0
      }]
    }
    if { !$found } {
      lappend missing $name
    }
  }
  return $missing
}

# The block's instances by placement: fixed, placed (movable) and
# unplaced, and how many of the storage flops (DFF*) are not fixed.
proc rf_placement { } {
  set n [dict create fixed 0 placed 0 unplaced 0 flops_not_fixed 0]
  foreach inst [[ord::get_db_block] getInsts] {
    set status [$inst getPlacementStatus]
    if { $status == "FIRM" || $status == "LOCKED" || $status == "COVER" } {
      dict incr n fixed
    } elseif { $status == "PLACED" || $status == "SUGGESTED" } {
      dict incr n placed
    } else {
      dict incr n unplaced
    }
    if {
      [string match "DFF*" [[$inst getMaster] getName]]
      && $status != "FIRM"
    } {
      dict incr n flops_not_fixed
    }
  }
  return $n
}
