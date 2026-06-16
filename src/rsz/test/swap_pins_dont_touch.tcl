# repair_timing -setup with SwapPinsMove must honor net dont-touch.
# Without the net-level guard in SwapPinsMove::swapPins(), disconnectPin()
# on a dont-touch net triggers ODB-0372 and crashes.
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

read_verilog swap_pins_dont_touch.v
link_design td1

create_clock [get_ports clk] -period 0.1
set_clock_uncertainty 0 [get_clocks clk]
set_input_delay -clock clk 0.02 [get_ports a1]
set_input_delay -clock clk 0.02 [get_ports a2]
set_input_delay -clock clk 0.02 [get_ports a3]
set_input_delay -clock clk 0.00 [get_ports a4]
set_input_delay -clock clk 0.00 [get_ports a5]
set_input_delay -clock clk 0.00 [get_ports a6]

set_output_delay -clock clk 0.01 [get_ports y1]
set_output_delay -clock clk 0.01 [get_ports y2]


# Load the placed fixture to keep this test independent of floorplan and placement.
read_def -floorplan_initialize swap_pins_dont_touch.def

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement

# Lock down net1 — it feeds U4.A4 which is the pin the pin-swap heuristic
# wants to move. SwapPinsMove must reject the swap rather than calling
# disconnectPin() on a dont-touch net.
set_dont_touch net1

report_worst_slack
repair_design
set_debug_level RSZ swap_pins_move 2
repair_timing -setup -verbose
set verilog_file [make_result_file swap_pins_dont_touch_out.v]
write_verilog $verilog_file
diff_files $verilog_file swap_pins_dont_touch_out.vok
