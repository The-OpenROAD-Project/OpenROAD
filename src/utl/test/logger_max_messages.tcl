source "helpers.tcl"

for {set i 0} {$i < 2000} {incr i} { utl::warn UTL 1234 "test" }
