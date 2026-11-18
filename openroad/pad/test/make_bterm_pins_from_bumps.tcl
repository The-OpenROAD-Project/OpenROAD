# Test for making bterm pins from the pad shapes of connected bumps.
# The port in0 has no geometry and its net reaches a bump: it receives the
# bump pad shape.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def make_bterm_pins_from_bumps.def

pad::make_bterm_pins_from_bumps [ord::get_db_block]

set def_file [make_result_file "make_bterm_pins_from_bumps.def"]
write_def $def_file
diff_files $def_file "make_bterm_pins_from_bumps.defok"
