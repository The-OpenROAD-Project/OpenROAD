# Regression test for ::tclreadline::complete, the Tcl-callable wrapper
# around the C++ TAB completion engine (ord::completeTcl) used by both
# the linenoise REPL and the web Tcl console.

# ---------------------------------------------------------------------------
# Command-position completion
# ---------------------------------------------------------------------------

set r [::tclreadline::complete "read_db" 7]
if { [lsearch -exact $r read_db] >= 0 } {
  puts "PASS: Test 1 candidate list contains 'read_db'"
} else {
  puts "FAIL: Test 1 candidate list: $r"
}

set r [::tclreadline::complete "place_" 6]
set all_place_prefix 1
foreach c $r {
  if { ![string match {place_*} $c] } { set all_place_prefix 0; break }
}
if { [llength $r] >= 3 && $all_place_prefix } {
  puts "PASS: Test 2 'place_' returns >=3 candidates all starting with place_"
} else {
  puts "FAIL: Test 2 'place_' candidates: $r"
}

set r [::tclreadline::complete "" 0]
if { [llength $r] >= 50 && [lsearch -exact $r read_lef] >= 0 } {
  puts "PASS: Test 3 empty line returns full command list (>=50, includes read_lef)"
} else {
  puts "FAIL: Test 3 empty line llength=[llength $r]"
}

# ---------------------------------------------------------------------------
# Flag (-arg) completion
# ---------------------------------------------------------------------------

set line "place_bondpad -"
set r [::tclreadline::complete $line [string length $line]]
set all_dash 1
foreach c $r {
  if { [string index $c 0] ne "-" } { set all_dash 0; break }
}
if { [llength $r] > 0 && $all_dash } {
  puts "PASS: Test 4 flag completion returns bare flags (each starts with '-')"
} else {
  puts "FAIL: Test 4 flag completion: $r"
}

set line "place_bondpad -b"
set r [::tclreadline::complete $line [string length $line]]
set all_match 1
foreach c $r {
  if { ![string match {-b*} $c] } { set all_match 0; break }
}
if { [llength $r] > 0 && $all_match } {
  puts "PASS: Test 5 flag narrowing 'place_bondpad -b' -> only -b* and not empty"
} else {
  puts "FAIL: Test 5 flag narrowing: $r"
}

# ---------------------------------------------------------------------------
# Variable completion
# ---------------------------------------------------------------------------

set ::ord_completion_test_var 1
set line "puts \$ord_completion_test_v"
set r [::tclreadline::complete $line [string length $line]]
set found 0
foreach c $r {
  if { $c eq "\$ord_completion_test_var" } { set found 1; break }
}
if { $found } {
  puts "PASS: Test 6 variable completion finds defined var with '\$' prefix"
} else {
  puts "FAIL: Test 6 variable completion: $r"
}

# ---------------------------------------------------------------------------
# File completion at argument position
# ---------------------------------------------------------------------------

set line "read_lef "
set r [::tclreadline::complete $line [string length $line]]
set found_self 0
foreach c $r {
  if { $c eq "ord_tclsh_completion.tcl" } { set found_self 1; break }
}
if { [llength $r] > 0 && $found_self } {
  puts "PASS: Test 7 file completion at arg position finds files in pwd"
} else {
  puts "FAIL: Test 7 file completion: llength=[llength $r] found_self=$found_self"
}

# ---------------------------------------------------------------------------
# Bracket boundary: text after '[' is a fresh command position
# ---------------------------------------------------------------------------

set line "puts \[set_"
set r [::tclreadline::complete $line [string length $line]]
set all_set_prefix 1
foreach c $r {
  if { ![string match {set_*} $c] } { set all_set_prefix 0; break }
}
if { [llength $r] > 0 && $all_set_prefix } {
  puts "PASS: Test 8 bracket boundary: '\[set_' completes as command position"
} else {
  puts "FAIL: Test 8 bracket boundary: $r"
}

# ---------------------------------------------------------------------------
# Plain Tcl builtins must be in the candidate list
# ---------------------------------------------------------------------------

set r [::tclreadline::complete "pu" 2]
if { [lsearch -exact $r puts] >= 0 } {
  puts "PASS: Test 9 'pu' includes the Tcl builtin 'puts'"
} else {
  puts "FAIL: Test 9 'pu' candidates: $r"
}

# ---------------------------------------------------------------------------
# User-defined procs at global scope must be in the candidate list
# ---------------------------------------------------------------------------

proc ord_completion_test_proc { } { }
set r [::tclreadline::complete "ord_completion_test_p" 21]
if { [lsearch -exact $r ord_completion_test_proc] >= 0 } {
  puts "PASS: Test 10 user proc 'ord_completion_test_proc' appears in candidates"
} else {
  puts "FAIL: Test 10 user proc candidates: $r"
}

# ---------------------------------------------------------------------------
# Flag completion must still work after a positional argument or flag value.
# Regression for the backward-walk bug in findEnclosingCommand.
# ---------------------------------------------------------------------------

set line "place_bondpad PAD -"
set r [::tclreadline::complete $line [string length $line]]
set all_dash 1
foreach c $r {
  if { [string index $c 0] ne "-" } { set all_dash 0; break }
}
if { [llength $r] > 0 && $all_dash } {
  puts "PASS: Test 11 flag completion after positional arg"
} else {
  puts "FAIL: Test 11 flag completion after positional arg: $r"
}
