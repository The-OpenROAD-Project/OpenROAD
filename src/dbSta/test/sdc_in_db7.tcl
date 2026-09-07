# read_db -sdc on a file that is not a database: the read fails, and no
# restore of constraints is attempted on the half-read design.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
close [open sdc_in_db7-empty.odb w]
catch { read_db -sdc sdc_in_db7-empty.odb } msg
file delete sdc_in_db7-empty.odb
puts "read_db failed: $msg"
puts "stored form: [ord::sdc_in_db_kind]"
