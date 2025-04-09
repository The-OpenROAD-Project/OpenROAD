# A minimal generic constraints.sdc for architectural exploration of macros
# -------------------------------------------------------------------------
# 
# Used in designs/asap7/mock-array, for example.
#
# From the following observations, all else follows: the only thing
# that can fail timing closure, is a register to register path. All
# other constraints give the flow an optimization target. Failure
# to meet the timing constraint of an optimization target constraint
# is not a timing closure failure.
# 
# Note that ORFS regression checks do not have the ability to distinguish
# between timing closure failures(register to register paths) and
# optimization constraints violations.
#
# Timing violations for optimization constraints
# in mock-array Element, such as maximum transit time for a combinational path
# through mock-array Element, may or may not cause timing
# violations later on higher up in mock-array on register to register paths.
# 
# For the Element, the only register to register path
# are within the Element and no lower level macros are
# involved. Register to register paths within Element have to be checked
# at the Element level as they are invisible higher up in mock-array.
# 
# As for the remaining optimization constraints for Element, they
# should be for combinational through paths(io-io) and
# from input pins to register(io-reg) and from register to output pins(reg-io):
# 
# This constraints.sdc file is designed such that the clock latency & tree
# can be ignored as far constraints go;
# it is not part of the optimization constraints. The clock tree latency
# is accounted for in register to register paths and not visible outside
# of the macro that use this constraints.sdc.
# 
# All non reg-reg paths in Element are part of reg-reg paths in mock-array
# and timing closure in which those take part are checked at the mock-array
# level.
# 
# With this in mind, the constraints.sdc file for the Element becomes
# quite general and simple. set_max_delay is used exclusively for
# optimization constraints and the clock period is used to check timing
# closure for reg-reg paths.
#
# set_input/output_delay are not used and can't be used
# with the requirement that the constraint.sdc file should be articulated
# without making assumptions about the clock tree insertion latency.
#
# To elaborate further: the time specified
# for set_input/output_delay is relative to the clock insertion point, i.e.
# the time at the clock pin for the macro, which makes it impossible to articulate
# the number that is passed in to set_input/output_delay without taking
# clock network insertion latency into account.
# 
# Since set_input_delay is not used and set_max_delay is used instead, then
# no hold cells are inserted, which is what is desired here.
#
# Details such as clock uncertainty, max transition time, load, etc.
# is beyond the scope of this generic constraints.sdc file.
# 
# Beware of [path segmentation](https://docs.xilinx.com/r/2020.2-English/ug906-vivado-design-analysis/TIMING-13-Timing-Paths-Ignored-Due-to-Path-Segmentation), which
# can occur with OpenSTA.

set sdc_version 2.0

# clk_name, clk_port_name and clk_period are set by the constraints.sdc file
# that includes this generic part.

set clk_port [get_ports $clk_port_name]
create_clock -period $clk_period -waveform [list 0 [expr $clk_period / 2]] -name $clk_name $clk_port

set non_clk_inputs  [lsearch -inline -all -not -exact [all_inputs] $clk_port]
set all_register_outputs [get_pins -of_objects [all_registers] -filter {direction == output}]

# Optimization targets: overconstrain by default and
# leave refinements to a more design specific constraints.sdc file.
#
# Minimum time for io-io, io-reg, reg-io paths in macro is on
# the order of 80ps for a small macro on ASAP7.
set_max_delay [expr {[info exists in2reg_max] ? $in2reg_max : 80}] -from $non_clk_inputs -to [all_registers]
set_max_delay [expr {[info exists reg2out_max] ? $reg2out_max : 80}] -from $all_register_outputs -to [all_outputs]
set_max_delay [expr {[info exists in2out_max] ? $in2out_max : 80}] -from $non_clk_inputs -to [all_outputs]

# This allows us to view the different groups
# in the histogram in the GUI and also includes these
# groups in the report
group_path -name in2reg -from $non_clk_inputs -to [all_registers]
group_path -name reg2out -from [all_registers] -to [all_outputs]
group_path -name reg2reg -from [all_registers] -to [all_registers]
group_path -name in2out -from $non_clk_inputs -to [all_outputs]
