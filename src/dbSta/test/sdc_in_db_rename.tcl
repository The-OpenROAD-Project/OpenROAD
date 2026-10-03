# An odb-only process (LEF, no liberty): read_db restores nothing, an
# instance the constraints refer to is renamed, and write_db carries the
# record through untouched -- now stale.
read_lef Nangate45/Nangate45.lef
read_db $::env(SDC_IN_DB_ODB)
puts "odb-only process, stored form: [ord::sdc_in_db_kind]"
[[ord::get_db_block] findInst r1] rename r1_renamed
write_db $::env(SDC_IN_DB_OUT)
puts "odb-only process, stored form after write_db: [ord::sdc_in_db_kind]"
