# The rules carry no resistance data for met4, which the design routes on at
# two widths, so with -lef_res its resistance comes from LEF alone.
source helpers.tcl

read_lef sky130hs/sky130hs.tlef
read_def met4_two_widths.def

set_extraction_rules_file rules_without_layer_resistance.rules
extract_parasitics -lef_res
