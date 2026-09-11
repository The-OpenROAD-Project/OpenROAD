# Second process of the sdc_in_db round-trip tests: the design and its
# constraints must come from the .odb alone. Reports what happened, then
# writes the restored constraints back out for the parent to diff.
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
if { [catch { read_db $::env(SDC_IN_DB_ODB) } msg] } {
  puts "read_db failed: $msg"
} else {
  puts "after read_db, stored form: [ord::sdc_in_db_kind]"
  write_sdc -no_timestamp $::env(SDC_IN_DB_SDC)
}
