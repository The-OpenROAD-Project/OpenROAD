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

sta::define_cmd_args "run_tool" {[-key1 key1] [-flag1] pos_arg1}

# Put helper functions in a separate namespace so they are not visible
# too users in the global namespace.
namespace eval tool {

proc tool_helper { } {
  puts "Helping 23/6"
}

}

# Example usage:
#  run_tool foo
#  run_tool -flag1 -key1 2.0 bar
#  help run_tool
proc run_tool { args } {
  sta::parse_key_args "run_tool" args \
    keys {-key1} flags {-flag1}

  if { [info exists keys(-key1)] } {
    set param1 $keys(-key1)
    sta::check_positive_float "-key1" $param1
    tool::tool_set_param1 $param1
  }

  tool::tool_set_flag1 [info exists flags(-flag1)]

  sta::check_argc_eq1 "run_tool" $args
  tool::tool_helper
  tool::tool_run [lindex $args 0]
}
