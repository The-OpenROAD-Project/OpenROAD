# Second process of sdc_in_db8: restore the constraints from the .odb,
# write the design again with them, and dump the record that write stored.
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_db $::env(SDC_IN_DB_ODB)
write_db -sdc $::env(SDC_IN_DB_OUT)
set fh [open $::env(SDC_IN_DB_RECORD) w]
fconfigure $fh -translation binary
puts -nonewline $fh \
  [[odb::dbStringProperty_find [ord::get_db_block] "sta.sdc.native"] getValue]
close $fh
