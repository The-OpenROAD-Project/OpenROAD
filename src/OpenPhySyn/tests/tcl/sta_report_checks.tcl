import_lib ../tests/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
import_lef ../tests/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
import_def ../tests/data/designs/gcd/gcd.def
sta create_clock [sta get_ports clk]  -name core_clock  -period 10
sta report_checks -path_delay min
exit 0