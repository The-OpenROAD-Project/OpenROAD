read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib

read_verilog eliminate_dead_logic1.v
link_design top

set_dont_touch \q2[0]
eliminate_dead_logic
write_verilog /dev/stdout
