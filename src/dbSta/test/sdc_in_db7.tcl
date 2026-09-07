# read_db -sdc on a file that is not a database: the read fails, and no
# restore of constraints is attempted on the half-read design.
source "sdc_in_db_common.tcl"
load_libs
# odb file ... is invalid: the message carries the platform's iostream text.
suppress_message ORD 54

close [open sdc_in_db7-empty.odb w]
set failed [catch { read_db -sdc sdc_in_db7-empty.odb } msg]
file delete sdc_in_db7-empty.odb
check "read_db failed" { set failed } 1
check "with the invalid-database error" { set msg } ORD-0054
check "nothing restored" { ord::sdc_in_db_kind } none
exit_summary
