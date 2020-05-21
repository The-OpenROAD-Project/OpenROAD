# resize to target_slew
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def reg2.def

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

sta::resizer_preamble [get_libs *]
if {0} {
set_load 30 u1z
sta::resize_instance_to_target_slew [get_cell u1]
report_instance u1
}

set_load 30 r1q
sta::resize_instance_to_target_slew [get_cell r1]
report_instance r1
