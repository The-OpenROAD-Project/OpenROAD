# Regression test for the case when the RESOVER table does not extend
# out to the 4 * pitch neighborhood window used by RCX and a wire has
# neighbors both inside and outside that table range.
source helpers.tcl

read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef

read_def gcd.def

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file short_resover.rules

set spef_file [make_result_file short_resover.spef]
write_spef $spef_file

diff_files short_resover.spefok $spef_file "^\\*(DATE|VERSION)"
