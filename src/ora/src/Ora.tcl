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

proc askbot { args } {
  sta::parse_key_args "askbot" args \
    flags {-listSources}

  ora::ora_set_listSources [info exists flags(-listSources)]

  sta::check_argc_eq1 "askbot" $args

  ora::askbot [lindex $args 0]
}

proc set_bothost { hostUrl } {
  puts "Setting ORAssistant host to $hostUrl"

  sta::check_argc_eq1 "set_bothost" $hostUrl

  ora::set_bothost $hostUrl
}