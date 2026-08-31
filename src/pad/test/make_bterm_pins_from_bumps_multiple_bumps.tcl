# Test for the port skip when making bterm pins from bumps.
# The port in1 has no geometry and two bumps on its net: the bump to copy the
# shape from would be an arbitrary choice, so it is reported and stays bare.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def make_bterm_pins_from_bumps_multiple_bumps.def

pad::make_bterm_pins_from_bumps [ord::get_db_block]

set def_file [make_result_file "make_bterm_pins_from_bumps_multiple_bumps.def"]
write_def $def_file
diff_files $def_file "make_bterm_pins_from_bumps_multiple_bumps.defok"
