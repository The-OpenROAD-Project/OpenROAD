# Check that check_power_grid takes the shapes of the net under test with 45
# degree edges as they are: an octagonal pin, which odb holds as boxes that
# reach outside of it, and a 45 degree wire, whose box is all of its
# bounding box. Shapes clear of the metal must not be reported, and shapes
# on the metal those boxes miss must be.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_lef short_pads.lef
read_def check_power_grid_short_45.def

catch { check_power_grid -net VDD -dont_require_terminals } err
puts $err
