#!/bin/sh
# The next line is executed by /bin/sh, but not Tcl \
exec tclsh $0 ${1+"$@"}

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

# Usage: MakeDatVar var_name var_file dat_file

set var [lindex $argv 0]
set var_file [lindex $argv 1]
set dat_file [lindex $argv 2]

set var_stream [open $var_file "w"]
puts $var_stream "#include <string>"
puts $var_stream "namespace stt::flt {"
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
