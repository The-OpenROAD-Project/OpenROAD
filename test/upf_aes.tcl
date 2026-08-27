source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_aes.v
link_design mpd_top
read_upf -file upf/mpd_aes.upf
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

set_domain_area PD_AES_1 -area {30   30 650 490}
set_domain_area PD_AES_2 -area {30 510 650 970}


initialize_floorplan \
  -die_area {0 0 1000 1000} \
  -core_area {30 30 970 970} \
  -site unithd \
  -additional_site unithddbl

make_tracks

set_routing_layers -signal li1-met5

place_pins \
  -hor_layers met3 \
  -ver_layers met2
global_placement -skip_initial_place -density uniform -routability_driven -timing_driven

detailed_placement -max_displacement 650
improve_placement
check_placement

set block [ord::get_db_block]
set regions [$block getRegions]

if { [llength $regions] != 2 } {
  utl::error "UPF" 1 "Expected 2 regions, found [llength $regions]"
}

set expected_regions [dict create "PD_AES_1" 0 "PD_AES_2" 0]

set dbu [$block getDefUnits]
foreach region $regions {
  set name [$region getName]
  set boundaries [$region getBoundaries]

  if { [llength $boundaries] != 1 } {
    utl::error "UPF" 5 "Region $name expected 1 box, found [llength $boundaries]"
  }

  if { [dict exists $expected_regions $name] } {
    dict incr expected_regions $name
  } else {
    utl::error "UPF" 4 "Unknown region $name"
  }

  set box [lindex $boundaries 0]
  set xmin [$box xMin]
  set ymin [$box yMin]
  set xmax [$box xMax]
  set ymax [$box yMax]

  if { [$region getRegionType] ne "EXCLUSIVE" } {
    utl::error "UPF" 7 "Region $name type mismatch: expected EXCLUSIVE, got [$region getRegionType]"
  }

  if { $name eq "PD_AES_1" } {
    if {
      $xmin != (30 * $dbu) || $ymin != (30 * $dbu)
      || $xmax != (650 * $dbu) || $ymax != (490 * $dbu)
    } {
      utl::error "UPF" 2 "Region PD_AES_1 boundary mismatch (in DBU): $xmin $ymin $xmax $ymax"
    }
  } elseif { $name eq "PD_AES_2" } {
    if {
      $xmin != (30 * $dbu) || $ymin != (510 * $dbu)
      || $xmax != (650 * $dbu) || $ymax != (970 * $dbu)
    } {
      utl::error "UPF" 3 "Region PD_AES_2 boundary mismatch (in DBU): $xmin $ymin $xmax $ymax"
    }
  }
}

dict for {name count} $expected_regions {
  if { $count != 1 } {
    utl::error "UPF" 6 "Expected exactly 1 region named $name, found $count"
  }
}

# If we got here without an error then every region assertion held.
puts pass
