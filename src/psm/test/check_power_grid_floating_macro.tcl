# Check that check_power_grid works with a disconnected macro
source helpers.tcl

read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7_data/fakeram7_256x32.lef
read_def asap7_data/riscv.def
catch {check_power_grid -net VDD} err
puts $err
