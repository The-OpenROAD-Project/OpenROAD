# write_db stores the timing constraints in the .odb, so an .odb is
# self-describing and does not have to be paired with a .sdc by name.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top
read_sdc hier1.sdc

set db_file [make_result_file sdc_in_db1.odb]
write_db $db_file

# The constraints stored in the block are what write_sdc would have written.
set block [ord::get_db_block]
set stored [odb::dbStringProperty_find $block "sta.sdc"]
set sdc_file [make_result_file sdc_in_db1.sdc]
write_sdc -no_timestamp $sdc_file
set fh [open $sdc_file r]
set written [read $fh]
close $fh
if { [$stored getValue] == $written } {
  puts "stored constraints match write_sdc"
} else {
  puts "MISMATCH"
}
