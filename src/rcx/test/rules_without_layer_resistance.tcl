# The rules carry no resistance data for met4, which the design routes on.
source helpers.tcl

read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def gcd.def

set_extraction_rules_file rules_without_layer_resistance.rules
catch { extract_parasitics }
