# -periphery unplaced: the core (storage and read trees) is placed, the
# periphery (decoders, select and input buffers) is left for a parent to
# place; the outline is the placed core and one row, and every pin sits
# inside it on a track of -pin_layer.

source "helpers.tcl"
source "ram_sim.tcl"

read_liberty ../../../test/asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty ../../../test/asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz

read_lef ../../../test/asap7/asap7_tech_1x_201209.lef
read_lef ../../../test/asap7/asap7sc7p5t_28_R_1x_220121a.lef


generate_ram_netlist \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -periphery unplaced -pin_layer M4

set block [ord::get_db_block]
set x0 1e18; set y0 1e18; set x1 -1e18; set y1 -1e18
set row_h 0
foreach inst [$block getInsts] {
  set name [$inst getName]
  if { [string match decoder* $name] || [string match *sel_buf* $name] } {
    ram_sim::check { ![$inst isPlaced] } "periphery $name is placed"
  }
  if { [string match storage_* $name] && [string match DFF* [[$inst getMaster] getName]] } {
    ram_sim::check { [$inst isPlaced] } "storage $name is not placed"
  }
  if { [$inst isPlaced] } {
    set b [$inst getBBox]
    set x0 [expr { min($x0, [$b xMin]) }]; set y0 [expr { min($y0, [$b yMin]) }]
    set x1 [expr { max($x1, [$b xMax]) }]; set y1 [expr { max($y1, [$b yMax]) }]
    set row_h [[$inst getMaster] getHeight]
  }
}
set die [$block getDieArea]
ram_sim::check { [$die xMin] <= $x0 && [$die yMin] <= $y0
                 && [$die xMax] >= $x1 && [$die yMax] >= $y1 } \
  "a placed cell is outside the outline"
ram_sim::check { [$die dy] - ($y1 - $y0) <= $row_h } \
  "the outline is [$die dy] tall for a core of [expr { $y1 - $y0 }]: more than a row over"
# M4 is horizontal: its tracks are rows, a pin's y on one of them and
# its x on the manufacturing grid
set m4 [[ord::get_db_tech] findLayer M4]
set pitch [$m4 getPitch]
set offset [$m4 getOffset]
set grid [[ord::get_db_tech] getManufacturingGrid]
foreach term [$block getBTerms] {
  foreach pin [$term getBPins] {
    foreach box [$pin getBoxes] {
      ram_sim::check { [[$box getTechLayer] getName] eq "M4" } "[$term getName] is not on M4"
      set x [expr { ([$box xMin] + [$box xMax]) / 2 }]
      set y [expr { ([$box yMin] + [$box yMax]) / 2 }]
      ram_sim::check { ($y - $offset) % $pitch == 0 } \
        "[$term getName] is off M4's tracks"
      ram_sim::check { $x % $grid == 0 } \
        "[$term getName] is off the manufacturing grid"
      ram_sim::check { [$box xMin] >= [$die xMin] && [$box xMax] <= [$die xMax] } \
        "[$term getName] is outside the outline"
    }
  }
}
ram_sim::load
ram_sim::exercise {
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
