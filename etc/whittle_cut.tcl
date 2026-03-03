# Cut a range of instances or nets from an ODB file.
#
# Environment variables:
#   WHITTLE_DB_INPUT   - path to the input ODB file
#   WHITTLE_DB_OUTPUT  - path to write the modified ODB file
#   WHITTLE_CUT_LEVEL  - "insts" or "nets"
#   WHITTLE_CUT_START  - start index (inclusive)
#   WHITTLE_CUT_END    - end index (exclusive)
#   WHITTLE_DEF_OUTPUT - (optional) path to write DEF output
#
# Prints to stdout:
#   INSTS <count>
#   NETS <count>

set db [odb::dbDatabase_create]
odb::read_db $db $env(WHITTLE_DB_INPUT)

set block [[$db getChip] getBlock]
set cut_level $env(WHITTLE_CUT_LEVEL)
set start $env(WHITTLE_CUT_START)
set end $env(WHITTLE_CUT_END)

if { $cut_level eq "insts" } {
  set elms [$block getInsts]
} else {
  set elms [$block getNets]
}

for { set i $start } { $i < $end } { incr i } {
  set elm [lindex $elms $i]
  if { $cut_level eq "insts" } {
    $elm setDoNotTouch false
    foreach iterm [$elm getITerms] {
      set net [$iterm getNet]
      if { $net ne "NULL" } {
        $net setDoNotTouch false
      }
    }
    odb::dbInst_destroy $elm
  } else {
    $elm setDoNotTouch false
    foreach iterm [$elm getITerms] {
      [$iterm getInst] setDoNotTouch false
    }
    odb::dbNet_destroy $elm
  }
}

odb::write_db $db $env(WHITTLE_DB_OUTPUT)

if { [info exists env(WHITTLE_DEF_OUTPUT)] } {
  odb::write_def $block $env(WHITTLE_DEF_OUTPUT)
}

set block [[$db getChip] getBlock]
puts "INSTS [llength [$block getInsts]]"
puts "NETS [llength [$block getNets]]"

exit
