source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_1024x32.lef
read_lef Nangate45/fakeram45_64x32.lef
read_def cut_rows.def

set db [::ord::get_db]
set blockages [tap::find_blockages $db]
tap::cut_rows $db FILLCELL_X1 $blockages 2 2
