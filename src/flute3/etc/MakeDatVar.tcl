#!/bin/sh
# The next line is executed by /bin/sh, but not Tcl \
exec tclsh $0 ${1+"$@"}

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

# Usage: MakeDatVar var_name var_file dat_file

set var [lindex $argv 0]
set var_file [lindex $argv 1]
set dat_file [lindex $argv 2]

set var_stream [open $var_file "w"]
puts $var_stream "#include <string>"
puts $var_stream "namespace Flute {"
puts -nonewline $var_stream "std::string $var = \""
close $var_stream

set b64_file "[file rootname $dat_file].b64"
set b64_file2 "[file rootname $dat_file].tr"

exec base64 -i $dat_file > $b64_file
# strip trailing \n from base64 file
exec tr -d "\n" <$b64_file >$b64_file2
exec cat < $b64_file2 >> $var_file

set var_stream [open $var_file "a"]
puts $var_stream "\";"
puts $var_stream "}"
close $var_stream

file delete $b64_file
file delete $b64_file2
