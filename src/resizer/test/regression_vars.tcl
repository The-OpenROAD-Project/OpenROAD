# Resizer, LEF/DEF gate resizer
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

# Regression variables.

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

# Record a test in the regression suite.
proc record_test { test cmd_dir } {
  global cmd_dirs test_groups
  set cmd_dirs($test) $cmd_dir
  lappend test_groups(all) $test
  return $test
}

# Record a test in the $RESIZER/test directory.
proc record_resizer_tests { tests } {
  global test_dir
  foreach test $tests {
    # Prune commented tests from the list.
    if { [string index $test 0] != "#" } {
      record_test $test $test_dir
    }
  }
}

# Record tests in $STAX/designs.
proc record_test_design { tests } {
  global env
  if [info exists env(STAX)] {
    foreach dir_test $tests {
      # Prune commented tests from the list.
      if { [string index $dir_test 0] != "#" } {
	if {[regexp {([a-zA-Z0-9_]+)/([a-zA-Z0-9_]+)} $dir_test \
	       ignore cmd_subdir test]} {
	  set cmd_dir [file join $env(STAX) "designs" $cmd_subdir]
	  record_test $test $cmd_dir
	} else {
	  puts "Warning: could not parse test name $dir_test"
	}
      }
    }
  }
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

################################################################

# Regression test lists.

# Record tests in resizer/test
record_resizer_tests {
  make_parasitics1
  make_parasitics2
  rebuffer1
  rebuffer2
  rebuffer4
  rebuffer7
  buffer_ports1
  buffer_ports2
  buffer_ports3
  resize1
  resize2
  resize4
  resize6
  repair_max_cap1
  repair_max_fanout1
  repair_max_slew1
  repair_max_slew2
  repair_max_slew3
  repair_hold1
  repair_hold2
  report_floating_nets1
  repair_tie_fanout1
  repair_tie_fanout2
  repair_tie_fanout3
}

# Record tests in $STAX/designs
record_test_design {
  aes2/aes2_rebuffer1
  aes2/aes2_resize1
  coyote/coyote_resize1
  jpeg/jpeg_resize1
  mea/mea_resize1
  vanilla_bean/vanilla_bean_resize1
}

################################################################

# Regression test groups

# Medium speed tests.
# run time <15s with optimized compile
define_test_group med {
  mea_resize1
  aes2_resize1
  aes2_rebuffer1
  vanilla_bean_resize1
}

define_test_group slow {
  jpeg_resize1
  coyote_resize1
}

set fast [group_tests all]
set fast [list_delete $fast [group_tests med]]
set fast [list_delete $fast [group_tests slow]]

define_test_group fast $fast
