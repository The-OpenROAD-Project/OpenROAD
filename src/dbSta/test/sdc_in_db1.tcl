# write_db stores the timing constraints in the .odb, so an .odb is
# self-describing and does not have to be paired with a .sdc by name.
# With nothing beyond what the native form carries, that form is used:
# every pin is an odb object id, so restoring it needs no name lookup.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top
read_sdc hier1.sdc

set db_file [make_result_file sdc_in_db1.odb]
write_db $db_file
puts "stored form: [ord::sdc_in_db_kind]"

set block [ord::get_db_block]
puts [[odb::dbStringProperty_find $block "sta.sdc.native"] getValue]
