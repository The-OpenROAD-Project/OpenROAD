# The command line -db filename is handed to tcl as a read_db command.
# The command is built with tcl list quoting, so a filename containing a
# brace, a space or a backslash reads.  Formatted as read_db {filename}
# it fails to parse for a name with an unbalanced brace, and the
# database is never read.
source "helpers.tcl"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_top.v
link_design mpd_top

set db_file [make_result_file "db_option_quoting od{d.odb"]
write_db $db_file

set openroad [info nameofexecutable]
set status 0
if {
  [catch {
    exec $openroad -no_init -no_splash -exit -db $db_file \
      db_option_quoting_check.tcl 2>@1
  } out]
} {
  set status [lindex $::errorCode 2]
}

set failures 0

proc check { description ok } {
  global failures out
  if { !$ok } {
    incr failures
    puts "FAIL: $description, openroad said:"
    puts $out
  }
}

check "the block is loaded from the -db file" \
  [string match {*block mpd_top*} $out]
check "openroad exits zero, status was $status" \
  [expr { $status == 0 }]

if { $failures == 0 } {
  puts pass
}
