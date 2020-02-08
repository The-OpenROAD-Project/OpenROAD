import_lib ../tests/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
import_lef ../tests/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
import_def ../tests/data/designs/gcd/gcd.def
sta create_clock [sta get_ports clk]  -name core_clock  -period 10
# sta report_checks

set_wire_rc 0.0020 0.00020
# set_log_level trace
set num_cloned [transform gate_clone 1.5 false]
puts "Cloned $num_cloned gates"
export_def ./outputs/cloned.def

# sta create_clock [sta get_ports clk]  -name core_clock  -period 10
# sta report_checks
exit 0
