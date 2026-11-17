# OBS and PORT LAYER SPACING / DESIGNRULEWIDTH modifiers survive a LEF round trip
source "helpers.tcl"

set db [ord::get_db]
read_lef "sky130hd/sky130hd.tlef"
read_lef "write_lef_obs_spacing.lef"
set lib [$db findLib write_lef_obs_spacing]

set out_lef [make_result_file "write_lef_obs_spacing.lef"]
set lef_write_result [odb::write_macro_lef $lib $out_lef]
if { $lef_write_result != 1 } {
  puts "FAIL: lef write error"
  exit 1
}

diff_files $out_lef "write_lef_obs_spacing.lefok"

puts "pass"
exit 0
