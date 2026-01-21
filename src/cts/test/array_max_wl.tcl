# Build an array of tiles with misc flops and hook up a clk net for CTS
# This demonstrates the trouble with blockage unaware fixing.

source "helpers.tcl"
source Nangate45/Nangate45.vars

# Settings
set array_size 15
set pitch_multiplier 1.1
set core_margin [expr 10 * 2000]
set halo [expr 1 * 2000]

read_lib $liberty_file
read_lib array_tile.lib

read_lef $tech_lef
read_lef $std_cell_lef
read_lef array_tile.lef

set db [ord::get_db]
set tech [ord::get_db_tech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip "top"]
$block setDefUnits 2000

set dbu [$tech getDbUnitsPerMicron]

set tile [$db findMaster "array_tile"]

set width [$tile getWidth]
set height [$tile getHeight]

set x_pitch [expr int($width * $pitch_multiplier)]
set y_pitch [expr int($height * $pitch_multiplier)]

set clk [odb::dbNet_create $block "clk"]
$clk setSigType CLOCK

# Make clock bterm/bpin at 0,0
set clk_bterm [odb::dbBTerm_create $clk "clk"]
$clk_bterm setSigType CLOCK
$clk_bterm setIoType INPUT
set clk_bpin [odb::dbBPin_create $clk_bterm]
$clk_bpin setPlacementStatus PLACED
set layer [$tech findLayer "metal5"]
odb::dbBox_create $clk_bpin $layer 0 0 2000 2000

# tile array
for { set x 0 } { $x < $array_size } { incr x } {
  for { set y 0 } { $y < $array_size } { incr y } {
    set inst [odb::dbInst_create $block $tile "inst_${x}_${y}"]
    set lx [expr $x * $x_pitch + $core_margin]
    set ly [expr $y * $y_pitch + $core_margin]
    $inst setLocation $lx $ly
    $inst setPlacementStatus LOCKED
    set clk_iterm [$inst findITerm "clk"]
    $clk_iterm connect $clk

    set ux [expr $lx + $width + $halo]
    set uy [expr $ly + $height + $halo]

    set lx [expr $lx - $halo]
    set ly [expr $ly - $halo]
    odb::dbBlockage_create $block $lx $ly $ux $uy

    # connect east/west pins skipping every fifth column
    if { $x > 0 } {
      if { ($x + 1) % 5 == 0 } {
        continue
      } elseif { $x % 5 == 0 } {
        set offset 2
      } else {
        set offset 1
      }
      set l_inst [$block findInst "inst_[expr $x-$offset]_${y}"]
      set net [odb::dbNet_create $block "n1_${x}_${y}"]
      [$inst findITerm "w_in"] connect $net
      [$l_inst findITerm "e_out"] connect $net

      set net [odb::dbNet_create $block "n2_${x}_${y}"]
      [$inst findITerm "w_out"] connect $net
      [$l_inst findITerm "e_in"] connect $net
    }
  }
}

# FF array on every 5th column
set dff [$db findMaster "DFF_X1"]
set dff_height [$dff getHeight]
set dff_width [$dff getWidth]

for { set x 4 } { $x < $array_size } { incr x 5 } {
  for { set y 0 } { $y < $array_size } { incr y } {
    for { set i 0 } { $i < 50 } { incr i } {
      set inst [odb::dbInst_create $block $dff "dff_${x}_${y}_${i}"]
      set lx [expr $x * $x_pitch + $core_margin]
      set ly [expr $y * $y_pitch + $core_margin]
      $inst setLocation \
        [expr $lx - $dff_width - $halo] \
        [expr $ly + $i * $dff_height]
      $inst setPlacementStatus PLACED
      set clk_iterm [$inst findITerm "CK"]
      $clk_iterm connect $clk
    }
  }
}

ord::design_created

set overall_width [expr ($array_size - 1) * $x_pitch + $width + 2 * $core_margin]
set overall_height [expr ($array_size - 1) * $y_pitch + $height + 2 * $core_margin]

set overall_width [expr $overall_width / $dbu]
set overall_height [expr $overall_height / $dbu]

# setup rows
initialize_floorplan \
  -die_area [list 0 0 $overall_width $overall_height] \
  -core_area [list 0 0 $overall_width $overall_height] \
  -site $site

source $tracks_file

source Nangate45/Nangate45.rc

create_clock -period 5 clk

source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

set_debug_level CTS "tech char" 3

set_cts_config -wire_unit 30 \
  -sink_clustering_max_diameter $cts_cluster_diameter \
  -root_buf $cts_buffer \
  -buf_list $cts_buffer

clock_tree_synthesis -sink_clustering_enable

set_propagated_clock [all_clocks]
estimate_parasitics -placement
repair_clock_nets

set_placement_padding -global -left $detail_place_pad -right $detail_place_pad
detailed_placement

estimate_parasitics -placement
report_clock_skew

# max latency path
report_checks -through inst_1_1/clk -format full_clock_expanded

# min latency path
report_checks -through inst_2_1/clk -format full_clock_expanded

set def_file [make_result_file array_max_wl.def]
write_def $def_file
diff_files array_max_wl.defok $def_file
