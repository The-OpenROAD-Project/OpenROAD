# Copyright (c) 2025, The Regents of the University of California
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

proc askbot { args } {
  sta::parse_key_args "askbot" args \
    flags {-listSources}

  ora::askbot_listSources [info exists flags(-listSources)]

  sta::check_argc_eq1 "askbot" $args

  ora::askbot [lindex $args 0]
}


proc ora_init {arg1 {arg2 ""}} {
  if {$arg1 ni {"local" "cloud"}} {
    puts "ERROR: Invalid mode '$arg1'. Use 'local' or 'cloud'."
    return;
  }

  ora::set_mode $arg1
  
  if {$arg1 eq "cloud"} {
    ora::set_consent $arg2
  } elseif {$arg1 eq "local"} {
    ora::set_bothost $arg2
  } else {
    puts "Invalid mode: $arg1"
  }
}
