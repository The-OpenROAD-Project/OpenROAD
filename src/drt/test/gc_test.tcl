source "helpers.tcl"

set test_dir [pwd]
set openroad_dir [file dirname [file dirname [file dirname $test_dir]]]
set test_path [file join $openroad_dir "build" "src" "drt" "trTest"]

set test_status [catch { exec sh -c "BASE_DIR=$test_dir $test_path" } output option]

puts $test_status 
puts $output 
if { $test_status != 0 } {
  set test_err_info [lassign [dict get $option -errorcode] err_type]
  switch -exact -- $err_type {
    NONE {
      #passed
    }
    CHILDSTATUS {
      # non-zero exit status
      set exit_status [lindex $test_err_info 1]
      puts "ERROR: test returned exit code $exit_status"
      exit 1

    }
    default {
      puts "ERROR: $option"
      exit 1
    }
  }
}
puts "pass"
exit 0
