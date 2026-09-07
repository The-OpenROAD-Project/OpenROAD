# Second process of the sdc_in_db round-trip tests: the design and its
# constraints must come from the .odb alone. Writes the restored
# constraints back out for the parent to diff against what it wrote.
read_lef liberty1.lef
read_liberty liberty1.lib
read_db -sdc $::env(SDC_IN_DB_ODB)
puts "after read_db -sdc, stored form: [ord::sdc_in_db_kind]"
write_sdc -no_timestamp $::env(SDC_IN_DB_SDC)
