# Copyright (c) 2021, The Regents of the University of California
# All rights reserved.
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

sta::define_cmd_args "run_ora" {[-key1 key1] [-flag1] query}

# Put helper functions in a separate namespace so they are not visible
# too users in the global namespace.
namespace eval ora {

proc ora_helper { } {
  puts "ORAssistant Helper Function"
}

}

# Example usage:
#  askbot foo
#  askbot -flag1 -key1 2.0 bar
#  help askbot 
proc askbot { args } {
  sta::parse_key_args "askbot" args \
    flags {-listSources -listContext}

  # if { [info exists keys(-key1)] } {
  #   set param1 $keys(-key1)
  #   sta::check_positive_float "-key1" $param1
  #   ora::ora_set_param1 $param1
  # }

  ora::ora_set_listSources [info exists flags(-listSources)]
  ora::ora_set_listContext [info exists flags(-listContext)]

  sta::check_argc_eq1 "askbot" $args
  ora::ora_helper
  ora::askbot [lindex $args 0]
}
