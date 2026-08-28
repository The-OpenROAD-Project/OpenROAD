# read_db -sdc replays the constraints stored in the .odb. No .sdc file is
# read here: the design arrives constrained.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_db -sdc sdc_in_db.odb

foreach clk [get_clocks *] {
  puts "clock [get_name $clk] period [get_property $clk period]"
}
set sdc_file [make_result_file sdc_in_db2.sdc]
write_sdc -no_timestamp $sdc_file
report_file $sdc_file
