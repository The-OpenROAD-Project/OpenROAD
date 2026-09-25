# Helper for init_file_error.tcl.  Runs as cmd_file after the .openroad
# init file, and reports whether the init file was read.
if { [info exists init_file_error_was_read] } {
  puts "init file was read"
}
puts "cmd_file ran"
