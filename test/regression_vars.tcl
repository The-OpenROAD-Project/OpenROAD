# Copied from OpenSTA/test/regression_vars.tcl
# Copyright (c) 2021, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# regression.tcl variables for OpenROAD.

# Application program to run tests on.
set app "openroad"
if { [info exist ::env(OPENROAD_EXE)] } {
  set app_path "$::env(OPENROAD_EXE)"
} else {
  set app_path [file join $openroad_dir "build" "src" $app]
}
# Application options.
set app_options "-no_init -no_splash -exit"
# Log files for each test are placed in result_dir.
set result_dir [file join $test_dir "results"]
# Collective diffs.
set diff_file [file join $result_dir "diffs"]
if { [info exist ::env(DIFF_LOCATION)] } {
  set diff_file "$::env(DIFF_LOCATION)"
}
# File containing list of failed tests.
set failure_file [file join $result_dir "failures"]
# Use the DIFF_OPTIONS envar to change the diff options
# (Solaris diff doesn't support this envar)
set diff_options "-c"
if [info exists env(DIFF_OPTIONS)] {
  set diff_options $env(DIFF_OPTIONS)
}

set valgrind_suppress [file join $openroad_dir "test" valgrind.suppress]
set valgrind_options "--num-callers=20 --leak-check=full --freelist-vol=100000000 --leak-resolution=high --suppressions=$valgrind_suppress"
if { [exec "uname"] == "Darwin" } {
  append valgrind_options " --dsymutil=yes"
}

proc cleanse_logfile { test log_file } {
  # Nothing to be done here.
}

################################################################

# Record tests in the /test directory.
# Compare results/<test>.log to <test>.ok for pass/fail.
proc record_tests { tests } {
  record_tests1 $tests "compare_logfile"
}

# Record tests in the /test directory.
# Last line of results/<test>.log should be pass/fail.
proc record_pass_fail_tests { tests } {
  record_tests1 $tests "pass_fail"
}

proc record_flow_tests { tests } {
  record_tests1 $tests "check_metrics"
  define_test_group "flow" $tests
}

proc record_tests1 { tests cmp_logfile } {
  global test_dir
  if { [info exist ::env(CTEST_TESTNAME)]} {
    set tests "$::env(CTEST_TESTNAME)"
    set cmp_logfile "$::env(TEST_TYPE)"
  }
  foreach test $tests {
    # Prune commented tests from the list.
    if { [string index $test 0] != "#" } {
      record_test $test $test_dir $cmp_logfile
    }
  }
}

# Record a test in the regression suite.
proc record_test { test cmd_dir pass_criteria } {
  global cmd_dirs test_groups test_pass_criteria test_langs
  set cmd_dirs($test) $cmd_dir
  lappend test_groups(all) $test
  set test_pass_criteria($test) $pass_criteria
  set test_langs($test) [list]
  if {[file exists [file join $cmd_dir "$test.tcl"]]} {
    lappend test_langs($test) tcl
  }

  if {[file exists [file join $cmd_dir "$test.py"]]} {
    lappend test_langs($test) py
  }
  return $test
}

################################################################

proc define_test_group { group tests } {
  global test_groups
  set test_groups($group) $tests
}

proc group_tests { group } {
  global test_groups
  if { [info exists test_groups($group)] } {
    return $test_groups($group)
  } else {
    return {}
  }
}

# Clear the test lists.
proc clear_tests {} {
  global test_groups
  unset test_groups
}

proc list_delete { list delete } {
  set result {}
  foreach item $list {
    if { [lsearch $delete $item] == -1 } {
      lappend result $item
    }
  }
  return $result
}
