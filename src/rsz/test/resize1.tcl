# resize to target_slew
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg2.def

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

rsz::resizer_preamble
set_load 30 r1q
rsz::resize_driver_to_target_slew [get_pin r1/Q]
report_instance r1
