# Check that check_power_grid reports the power net shorting to routing
# obstructions: a plain metal1 blockage (dbObstruction), a blockage owned
# by a component (dbObstruction with an instance), and the master
# obstructions of two placed macros (dbMaster OBS, on metal1 over the rail
# and on metal2 over a strap).
#
# The blockage clear of the rail, the blockage on a layer VDD does not use,
# the EXCEPTPGNET blockage, the placement blockage and the macro placed
# clear of the grid are controls and must never be reported.
#
# The blockage of u6 is reported twice, once per VDD shape it lands on:
# the followpin rail, clipped to it, and the metal1 stub of u6's own VDD
# pin, which runs from the rail up to y = 4.225um and so covers the whole
# height of the blockage.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_lef short_obs.lef
read_def check_power_grid_short_obstructions.def

catch { check_power_grid -net VDD -dont_require_terminals } err
puts $err
