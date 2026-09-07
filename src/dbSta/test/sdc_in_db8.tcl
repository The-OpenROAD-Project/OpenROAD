# The native record is canonical and restoring it is a fixpoint: the same
# constraints give the same bytes from a second write_db in the same
# process and from a write_db in a fresh process that restored them from
# the .odb, and so do the .odb files themselves.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog reg1.v
link_design top
read_sdc sdc_in_db4.sdc

proc native_record { } {
  return [[odb::dbStringProperty_find [ord::get_db_block] "sta.sdc.native"] getValue]
}
proc file_bytes { path } {
  set fh [open $path r]
  fconfigure $fh -translation binary
  set bytes [read $fh]
  close $fh
  return $bytes
}
proc verdict { same } {
  return [expr { $same ? "identical" : "DIFFERENT" }]
}

set a [make_result_file sdc_in_db8_a.odb]
set b [make_result_file sdc_in_db8_b.odb]
set c [make_result_file sdc_in_db8_c.odb]
write_db $a
set record_a [native_record]
write_db $b
set record_b [native_record]

set ::env(SDC_IN_DB_ODB) $a
set ::env(SDC_IN_DB_OUT) $c
set ::env(SDC_IN_DB_RECORD) [make_result_file sdc_in_db8_c.native]
exec $argv0 -no_init -no_splash -exit sdc_in_db_rewrite.tcl
set record_c [file_bytes $::env(SDC_IN_DB_RECORD)]

puts "record, second write_db same process: [verdict [expr {$record_a eq $record_b}]]"
puts "record, write_db after restore in a fresh process: [verdict [expr {$record_a eq $record_c}]]"
puts ".odb, second write_db same process: [verdict [expr {[file_bytes $a] eq [file_bytes $b]}]]"
puts ".odb, write_db after restore in a fresh process: [verdict [expr {[file_bytes $a] eq [file_bytes $c]}]]"
puts "record lines: [llength [split [string trim $record_a] \n]]"
