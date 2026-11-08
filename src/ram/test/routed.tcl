# generate_ram's arguments for a routed block on the tests' technologies,
# and the check that the block came out routed.

namespace eval routed {
# asap7: the libraries the asap7 tests read, and its strap widths, which
# must be ones the technology allows. The technology LEF's M2 track offset
# is negative, so the tracks come from the file asap7's flows use.
proc read_asap7 { } {
  read_liberty ../../../test/asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
  read_liberty ../../../test/asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
  read_liberty ../../../test/asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
  read_liberty ../../../test/asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
  read_liberty ../../../test/asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz

  read_lef ../../../test/asap7/asap7_tech_1x_201209.lef
  read_lef ../../../test/asap7/asap7sc7p5t_28_R_1x_220121a.lef
}

variable asap7 {
  -routing_layer {M1 0.018}
  -ver_layer {M3 0.090 2.16}
  -hor_layer {M4 0.120 1.08}
  -filler_cells {FILLERxp5_ASAP7_75t_R FILLER_ASAP7_75t_R}
  -tapcell TAPCELL_ASAP7_75t_R
  -max_tap_dist 25
  -tracks ../../../test/asap7/asap7.tracks.tcl
}

proc read_nangate45 { } {
  read_liberty Nangate45/Nangate45_typ.lib
  read_lef Nangate45/Nangate45_tech.lef
  read_lef Nangate45/Nangate45.lef
}

variable nangate45 {
  -routing_layer {metal1 0.08}
  -ver_layer {metal4 0.14 9}
  -hor_layer {metal3 0.08 8}
  -filler_cells {FILLCELL_X1 FILLCELL_X2 FILLCELL_X4 FILLCELL_X8}
  -tapcell TAPCELL_X1
  -max_tap_dist 10
}

# Every signal net with two or more pins has a wire, detailed routing
# left no violations, and no cell has a signal input left open.
proc check { } {
  set block [ord::get_db_block]
  foreach inst [$block getInsts] {
    foreach iterm [$inst getITerms] {
      if { [$iterm getNet] == "NULL" && [$iterm isInputSignal] } {
        error "[$inst getName]/[[$iterm getMTerm] getName] is not connected"
      }
    }
  }
  set nets 0
  foreach net [$block getNets] {
    if { [$net getSigType] != "SIGNAL" } {
      continue
    }
    if { [llength [$net getITerms]] + [llength [$net getBTerms]] < 2 } {
      continue
    }
    incr nets
    if { [$net getWire] == "NULL" } {
      error "net [$net getName] is not routed"
    }
  }
  if { $nets == 0 } {
    error "no signal nets to route"
  }
  set drc [$block findMarkerCategory "DRC"]
  if { $drc == "NULL" } {
    error "no DRC markers: detailed routing did not run"
  }
  if { [$drc getMarkerCount] > 0 } {
    error "[$drc getMarkerCount] DRC violations"
  }
}
}
