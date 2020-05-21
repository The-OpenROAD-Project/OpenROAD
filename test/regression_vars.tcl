# Copyright (c) 2019, Parallax Software, Inc.
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
set app_path [file join $openroad_dir "build" "src" $app]
# Application options.
set app_options "-no_init -no_splash -exit"
# Log files for each test are placed in result_dir.
set result_dir [file join $test_dir "results"]
# Collective diffs.
set diff_file [file join $result_dir "diffs"]
# File containing list of failed tests.
set failure_file [file join $result_dir "failures"]
# Use the DIFF_OPTIONS envar to change the diff options
# (Solaris diff doesn't support this envar)
set diff_options "-c"
if [info exists env(DIFF_OPTIONS)] {
  set diff_options $env(DIFF_OPTIONS)
}

set valgrind_suppress [file join $test_dir valgrind.suppress]
set valgrind_options "--num-callers=20 --leak-check=full --freelist-vol=100000000 --leak-resolution=high --suppressions=$valgrind_suppress"
if { [exec "uname"] == "Darwin" } {
  append valgrind_options " --dsymutil=yes"
}

proc cleanse_logfile { test log_file } {
  # Nothing to be done here.
}

################################################################

# Record tests in the /test directory.
proc record_tests { tests } {
  record_tests1 $tests 1
}

proc record_flow_tests { tests } {
  record_tests1 $tests 0
}

proc record_tests1 { tests cmp_logfile } {
  global test_dir
  foreach test $tests {
    # Prune commented tests from the list.
    if { [string index $test 0] != "#" } {
      record_test $test $test_dir $cmp_logfile
    }
  }
}

# Record a test in the regression suite.
proc record_test { test cmd_dir cmp_logfile } {
  global cmd_dirs test_groups compare_logfile
  set cmd_dirs($test) $cmd_dir
  lappend test_groups(all) $test
  set compare_logfile($test) $cmp_logfile
  return $test
}

################################################################

proc define_test_group { name tests } {
  global test_groups
  set test_groups($name) $tests
}

proc group_tests { name } {
  global test_groups
  return $test_groups($name)
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
