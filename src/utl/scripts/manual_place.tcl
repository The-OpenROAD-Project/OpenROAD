# Proc: _place_line
# Internal helper to handle row/column placement logic.
proc _place_line { orientation instances startX startY spacing status } {
  set db [ord::get_db]
  set block [[$db getChip] getBlock]
  set currentX $startX
  set currentY $startY

  foreach instName $instances {
    set inst [$block findInst $instName]
    if { $inst == "NULL" } {
      utl::warn "UTL" 4043 "Instance $instName not found. Skipping."
      continue
    }

    $inst setOrigin $currentX $currentY
    $inst setPlacementStatus $status

    set master [$inst getMaster]
    if { $orientation == "row" } {
      set width [$master getWidth]
      set currentX [expr { $currentX + $width + $spacing }]
    } else {
      set height [$master getHeight]
      set currentY [expr { $currentY + $height + $spacing }]
    }
  }
}

# Proc: place_row
# Description: Places a list of instances in a horizontal row.
proc place_row { args } {
  parse_key_args "place_row" args \
    keys {-instances -startX -startY -spacing -status} \
    flags {}

  set spacing [expr { [info exists keys(-spacing)] ? $keys(-spacing) : 0 }]
  set status [expr { [info exists keys(-status)] ? $keys(-status) : "FIRM" }]

  _place_line "row" $keys(-instances) $keys(-startX) $keys(-startY) $spacing $status
}

# Proc: place_column
# Description: Places a list of instances in a vertical column.
proc place_column { args } {
  parse_key_args "place_column" args \
    keys {-instances -startX -startY -spacing -status} \
    flags {}

  set spacing [expr { [info exists keys(-spacing)] ? $keys(-spacing) : 0 }]
  set status [expr { [info exists keys(-status)] ? $keys(-status) : "FIRM" }]

  _place_line "column" $keys(-instances) $keys(-startX) $keys(-startY) $spacing $status
}
