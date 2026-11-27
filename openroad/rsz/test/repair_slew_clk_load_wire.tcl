# repair_design buffers a long wire against a clock -data_path load slew limit.
# repair_slew15 with the 0.08 limit moved from [current_design] to the
# clock data path: u2 drives a 10x-resistance wire whose far loads (u11/A)
# see ~0.12 slew, under the Nangate45 input default (0.198) but over the
# clock limit.  The load-slew tree (BufferedNet) must take its limit from
# the load pins' clocks, as checkLoadSlews does, or it inserts no buffer.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew10.def

create_clock -name clk -period 1
set_input_delay 0 -clock clk [get_ports in1]
set_max_transition 0.08 -data_path [get_clocks clk]

# 10x wire resistance
set_wire_rc -resistance 3.574e-02 -capacitance 7.516e-02
estimate_parasitics -placement

proc pin_slew { pin_name } {
  return [get_property [get_pins $pin_name] slew_max]
}

report_check_types -max_slew -violators -digits 3
check "u11/A violates the clock data-path limit" \
  { expr { [pin_slew u11/A] > 0.08 } } 1
check "u11/A meets the library input default" \
  { expr { [pin_slew u11/A] < 0.198 } } 1

set insts_before [llength [get_cells *]]
repair_design
report_check_types -max_slew -violators -digits 3

check "a repeater is inserted on the long wire" \
  { expr { [llength [get_cells *]] > $insts_before } } 1
check "u11/A meets the clock data-path limit" \
  { expr { [pin_slew u11/A] <= 0.08 } } 1
check "no max slew violations remain" { sta::max_slew_violation_count } 0

exit_summary
