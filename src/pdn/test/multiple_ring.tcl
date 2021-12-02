source "helpers.tcl"

read_lef ../../../test/sky130hvl/sky130_fd_sc_hvl.tlef 
read_lef ../../../test/sky130hvl/sky130_fd_sc_hvl_merged.lef 

read_lef sky130hvl/capacitor_test_nf.lef  
read_lef sky130hvl/LDO_COMPARATOR_LATCH.lef
read_lef sky130hvl/PMOS.lef
read_lef sky130hvl/PT_UNIT_CELL.lef
read_lef sky130hvl/vref_gen_nmos_with_trim.lef

read_def multiple_ring/2_5_floorplan_tapcell.def

pdngen multiple_ring/pdn.cfg -verbose

set def_file results/multiple_ring.def
write_def $def_file 

diff_files $def_file multiple_ring.defok
