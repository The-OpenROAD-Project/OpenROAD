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

read_db $env(WHITTLE_DB_INPUT)

set block [ord::get_db_block]
set cut_level $env(WHITTLE_CUT_LEVEL)
set start $env(WHITTLE_CUT_START)
set end $env(WHITTLE_CUT_END)

if { $cut_level eq "insts" } {
  set elms [$block getInsts]
} else {
  set elms [$block getNets]
}

foreach elm [lrange $elms $start [expr { $end - 1 }]] {
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

write_db $env(WHITTLE_DB_OUTPUT)

if { [info exists env(WHITTLE_DEF_OUTPUT)] } {
  odb::write_def $block $env(WHITTLE_DEF_OUTPUT)
}

puts "INSTS [llength [$block getInsts]]"
puts "NETS [llength [$block getNets]]"

exit
