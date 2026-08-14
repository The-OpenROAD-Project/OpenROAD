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

# ---------------------------------------------------------------------------
# Namespace variable completion: completing the prefix of a child namespace
# must produce "$ns::" (with trailing ::) so the next TAB drives variable
# completion inside that namespace, not the bare "$ns" which is not a valid
# variable reference on its own.
# ---------------------------------------------------------------------------

namespace eval ord_completion_test_ns { variable hello 1 }
set line "puts \$ord_completion_test_n"
set r [::tclreadline::complete $line [string length $line]]
set found_qualified 0
set found_bare 0
foreach c $r {
  if { $c eq "\$ord_completion_test_ns::" } { set found_qualified 1 }
  if { $c eq "\$ord_completion_test_ns" } { set found_bare 1 }
}
if { $found_qualified && !$found_bare } {
  puts "PASS: Test 12 namespace variable completion produces '\$ns::' not bare '\$ns'"
} else {
  puts "FAIL: Test 12 namespace variable: q=$found_qualified bare=$found_bare r=$r"
}

# ---------------------------------------------------------------------------
# Namespace candidates must be prefix-filtered: typing "$ord_completion_test_n"
# should not pull in unrelated top-level namespaces like $sta::, $odb::.
# ---------------------------------------------------------------------------

set line "puts \$ord_completion_test_n"
set r [::tclreadline::complete $line [string length $line]]
set has_unrelated 0
foreach c $r {
  if { ![string match {$ord_completion_test_*} $c] } {
    set has_unrelated 1; break
  }
}
if { !$has_unrelated && [llength $r] > 0 } {
  puts "PASS: Test 13 namespace candidates are prefix-filtered"
} else {
  puts "FAIL: Test 13 namespace candidates not filtered: $r"
}

# ---------------------------------------------------------------------------
# Completion inside a namespace: "$ord_completion_test_ns::" + TAB must
# surface variables defined in that namespace (regression for "no
# completions from this namespace").
# ---------------------------------------------------------------------------

set line "puts \$ord_completion_test_ns::"
set r [::tclreadline::complete $line [string length $line]]
set found_var 0
foreach c $r {
  if { $c eq "\$ord_completion_test_ns::hello" } { set found_var 1; break }
}
if { $found_var } {
  puts "PASS: Test 14 completion inside namespace surfaces variables"
} else {
  puts "FAIL: Test 14 completion inside namespace: $r"
}

# ---------------------------------------------------------------------------
# Sub-namespace enumeration: "$ord_completion_test_outer::" must surface
# child namespaces with their own trailing "::".
# ---------------------------------------------------------------------------

namespace eval ord_completion_test_outer {
namespace eval inner { variable x 1 }
}
set line "puts \$ord_completion_test_outer::"
set r [::tclreadline::complete $line [string length $line]]
set found_child 0
foreach c $r {
  if { $c eq "\$ord_completion_test_outer::inner::" } { set found_child 1; break }
}
if { $found_child } {
  puts "PASS: Test 15 sub-namespace enumeration produces '\$outer::inner::'"
} else {
  puts "FAIL: Test 15 sub-namespace enumeration: $r"
}

# ---------------------------------------------------------------------------
# Command-only namespace fallback: when the typed `$<ns>::` namespace has no
# variables and no child namespaces, fall back to commands so TAB is not a
# dead end.  Emit commands WITHOUT the leading `$` so accepting the
# candidate produces valid Tcl (replaces the entire `$<prefix>` token).
# ---------------------------------------------------------------------------

namespace eval ord_completion_test_cmds {
proc inner_cmd { } { }
}
set line "puts \$ord_completion_test_cmds::"
set r [::tclreadline::complete $line [string length $line]]
set found_cmd 0
set has_dollar 0
foreach c $r {
  if { $c eq "ord_completion_test_cmds::inner_cmd" } { set found_cmd 1 }
  if { [string index $c 0] eq "\$" } { set has_dollar 1 }
}
if { $found_cmd && !$has_dollar } {
  puts "PASS: Test 16 command-only namespace falls back to commands (no '\$')"
} else {
  puts "FAIL: Test 16 command-only namespace: found_cmd=$found_cmd has_dollar=$has_dollar r=$r"
}

# ---------------------------------------------------------------------------
# File completion with a non-empty leaf: every candidate must start with the
# typed leaf.  Regression for an inverted starts_with filter that previously
# kept the non-matching entries and dropped the matching ones — Test 7 only
# exercises the empty-leaf path, so this is the case that caught the bug.
# ---------------------------------------------------------------------------

set line "read_lef ord_"
set r [::tclreadline::complete $line [string length $line]]
set all_prefix 1
set found_self 0
foreach c $r {
  if { ![string match {ord_*} $c] } { set all_prefix 0 }
  if { $c eq "ord_tclsh_completion.tcl" } { set found_self 1 }
}
if { [llength $r] > 0 && $all_prefix && $found_self } {
  puts "PASS: Test 17 file completion with non-empty leaf filters to 'ord_*'"
} else {
  puts "FAIL: Test 17 file completion non-empty leaf: prefix=$all_prefix self=$found_self r=$r"
}
