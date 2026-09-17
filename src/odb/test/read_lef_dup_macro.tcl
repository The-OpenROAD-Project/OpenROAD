# A MACRO redefined after its first definition has been frozen must be
# reported and dropped, not merged into the existing master.
source "helpers.tcl"

set db [ord::get_db]
read_lef "data/lef_dup_macro.lef"

set lib [lindex [$db getLibs] 0]

check "master count" {llength [$lib getMasters]} 2

set master [$lib findMaster DUP]
check "duplicate master found" {expr { $master != "NULL" }} 1

# Everything below comes from the first definition; the second one sets a
# different class, size and pin list.
check "master type" {$master getType} "CORE"
check "master width" {$master getWidth} 1000
check "master height" {$master getHeight} 1000
check "master term count" {$master getMTermCount} 1
check "master term names" {
  set names {}
  foreach mterm [$master getMTerms] { lappend names [$mterm getName] }
  set names
} "A"
check "master site" {[$master getSite] getName} core

# Parsing recovers: the macro following the duplicate is still read.
set after [$lib findMaster AFTER_DUP]
check "macro after duplicate found" {expr { $after != "NULL" }} 1
check "macro after duplicate width" {$after getWidth} 1000

exit_summary
