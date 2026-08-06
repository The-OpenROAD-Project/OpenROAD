# top layer pin placement must keep at least the layer min spacing from PDN
# shapes when the user keepout is smaller than it
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef blocked_region.lef

read_def blocked_region.def

set block [ord::get_db_block]
set met5 [[ord::get_db_tech] findLayer met5]

# wide strap crossing the pin pattern region
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $met5 50000 125000 250000 130000 "STRIPE"

# the first slot row ends 0.8um below the strap: allowed by the 0.3um user
# keepout, but closer than the layer min spacing
define_pin_shape_pattern -layer met5 -x_step 6.8 -y_step 6.8 \
  -region { 50 122.95 250 143 } -size { 1.6 2.5 } -pin_keepout 0.3
set_io_pin_constraint \
  -pin_names {clk resp_val req_val resp_rdy reset req_rdy} -region "up:*"

place_pins -hor_layer met3 -ver_layer met2

# count top layer pin shapes closer to the strap than the required spacing
proc count_strap_spacing_violations { } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        if { [$layer getName] != "met5" } {
          continue
        }
        set pin_length [expr {
          max([$box xMax] - [$box xMin], [$box yMax] - [$box yMin])
        }]
        set spacing [$layer getSpacing 10000 $pin_length]
        set dx [expr {
          max(0, max(50000 - [$box xMax], [$box xMin] - 250000))
        }]
        set dy [expr {
          max(0, max(125000 - [$box yMax], [$box yMin] - 130000))
        }]
        if { $dx < $spacing && $dy < $spacing } {
          puts "pin [$bterm getName] at\
            ([ord::dbu_to_microns [$box xMin]]\
            [ord::dbu_to_microns [$box yMin]])\
            ([ord::dbu_to_microns [$box xMax]]\
            [ord::dbu_to_microns [$box yMax]]) within\
            [ord::dbu_to_microns [expr { max($dx, $dy) }]]um of the strap,\
            needs [ord::dbu_to_microns $spacing]um"
          incr violations
        }
      }
    }
  }
  return $violations
}

puts "pin to strap spacing violations: [count_strap_spacing_violations]"
