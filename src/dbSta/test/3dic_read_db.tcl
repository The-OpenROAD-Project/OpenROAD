source "helpers.tcl"

# write_db/read_db round-trip of a 3DIC design. The restore path arrives via
# postReadDb (not postRead3Dbx); the 3DIC timing network must be initialized
# there too, so the restored design times identically.
read_3dbx 3dic_cross.3dbx

write_db [make_result_file 3dic_read_db.odb]

# Reload in a separate process so nothing from read_3dbx is reused.
puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  -threads [thread_count] \
  [file join [file dirname [info script]] "3dic_read_db_reload.tcl"] 2>@1]
