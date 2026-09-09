# top layer pin placement must keep at least the layer min spacing from PDN
# shapes when the user keepout is smaller than it
source "helpers.tcl"
source "pdn_helpers.tcl"

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

puts "pin to strap spacing violations: [count_pdn_shape_violations table]"
