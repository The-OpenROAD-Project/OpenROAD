proc read_macro_placement {macro_placement_file} {
  set block [ord::get_db_block]
  set units [$block getDefUnits]

  set ch [open $macro_placement_file]

  while {![eof $ch]} {
    set line [gets $ch]
    if {[llength $line] == 0} {continue}

    set inst_name [lindex $line 0]
    set orientation [lindex $line 1]
    set x [expr round([lindex $line 2] * $units)]
    set y [expr round([lindex $line 3] * $units)]

    if {[set inst [$block findInst $inst_name]] == "NULL"} {
      error "Cannot find instance $inst_name"
    }

    $inst setOrient $orientation
    $inst setOrigin $x $y
    $inst setPlacementStatus FIRM
  }

  close $ch
}
