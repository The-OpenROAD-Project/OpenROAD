source "helpers.tcl"

set test_dir [pwd]
set openroad_dir [file dirname [file dirname [file dirname $test_dir]]]
set tests_path [file join $openroad_dir "build" "src" "OpenDB" "test" "cpp"]

set tests_list [lsort [split [exec sh -c "find $tests_path -maxdepth 1 -name 'Test*'"] \n]]

foreach test $tests_list {
    set test_name [file tail $test]
    puts "Running test: $test_name"
    puts "**********"

    set test_status [catch { exec sh -c "BASE_DIR=$test_dir $test" } output option]

    puts $test_status 
    puts $output
    if { $test_status == 0 } {
        puts "$test_name passed" 
    } else {
        set test_err_info [lassign [dict get $option -errorcode] err_type]
        switch -exact -- $err_type {
        NONE {
            puts "$test_name passed"
        }
        CHILDSTATUS {
            # non-zero exit status
            set exit_status [lindex $test_err_info 1]
            set process_id  [lindex $test_err_info 0]

            puts "ERROR: test returned exit code $exit_status"
            exit 1

        }
        default {
            puts "ERROR: $option"
            exit 1
        }
    }
    }

    puts "******************"
}

puts "pass"
exit 0
