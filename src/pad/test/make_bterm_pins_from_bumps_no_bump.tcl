# Test for the port skip when making bterm pins from bumps.
# The port out0 has no geometry and no bump on its net: there is nothing to
# copy a shape from, so it is reported and stays bare.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def make_bterm_pins_from_bumps_no_bump.def

pad::make_bterm_pins_from_bumps [ord::get_db_block]

set def_file [make_result_file "make_bterm_pins_from_bumps_no_bump.def"]
write_def $def_file
diff_files $def_file "make_bterm_pins_from_bumps_no_bump.defok"
