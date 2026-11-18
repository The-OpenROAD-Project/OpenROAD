# Helper procs for the boundary PDN tests. A violation is a signal pin shape
# closer to a blocking shape than the required spacing: "overlap" requires
# plain intersection, "spacing" uses the layer default spacing, "table" the
# width-dependent spacing for wide shapes and an integer an explicit value.

proc get_required_spacing { layer pin_box llx lly urx ury mode } {
  if { $mode == "overlap" } {
    return 0
  }
  if { [string is integer -strict $mode] } {
    return $mode
  }
  if { $mode == "table" } {
    set pin_dx [expr { [$pin_box xMax] - [$pin_box xMin] }]
    set pin_dy [expr { [$pin_box yMax] - [$pin_box yMin] }]
    # like the placer, use the width of the widest shape, pin included
    set shape_width \
      [expr { max(min($urx - $llx, $ury - $lly), min($pin_dx, $pin_dy)) }]
    set pin_length [expr { max($pin_dx, $pin_dy) }]
    return [$layer getSpacing $shape_width $pin_length]
  }
  return [$layer getSpacing]
}

proc is_pin_shape_violation { pin_box llx lly urx ury required } {
  set dx [expr { max($llx - [$pin_box xMax], [$pin_box xMin] - $urx) }]
  set dy [expr { max($lly - [$pin_box yMax], [$pin_box yMin] - $ury) }]
  if { $dx < 0 && $dy < 0 } {
    return 1
  }
  return [expr { max($dx, 0) < $required && max($dy, 0) < $required }]
}

proc report_pin_violation { bterm box what } {
  puts "pin [$bterm getName] on layer [[$box getTechLayer] getName] at\
    ([ord::dbu_to_microns [$box xMin]] [ord::dbu_to_microns [$box yMin]])\
    ([ord::dbu_to_microns [$box xMax]] [ord::dbu_to_microns [$box yMax]])\
    too close to $what"
}

# count signal pin shapes too close to the given shapes, a list of
# {layer_name llx lly urx ury label} entries
proc count_violations_against { shapes mode } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" || [$bterm getSigType] == "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        foreach shape $shapes {
          lassign $shape layer_name llx lly urx ury label
          if { [$layer getName] != $layer_name } {
            continue
          }
          set required \
            [get_required_spacing $layer $box $llx $lly $urx $ury $mode]
          if { [is_pin_shape_violation $box $llx $lly $urx $ury $required] } {
            report_pin_violation $bterm $box $label
            incr violations
          }
        }
      }
    }
  }
  return $violations
}

# count signal pin shapes too close to same layer special net wires
proc count_pdn_shape_violations { mode } {
  set block [ord::get_db_block]
  set shapes {}
  foreach net [$block getNets] {
    if { ![$net isSpecial] } {
      continue
    }
    foreach swire [$net getSWires] {
      foreach sbox [$swire getWires] {
        set slayer [$sbox getTechLayer]
        if { $slayer == "NULL" } {
          continue
        }
        lappend shapes [list [$slayer getName] [$sbox xMin] [$sbox yMin] \
          [$sbox xMax] [$sbox yMax] "[$net getName] wire"]
      }
    }
  }
  return [count_violations_against $shapes $mode]
}

# count signal pin shapes on the given layer too close to the given rects
proc count_rect_violations { layer_name rects mode } {
  set shapes {}
  foreach rect $rects {
    lassign $rect llx lly urx ury
    lappend shapes \
      [list $layer_name $llx $lly $urx $ury "shape ($llx $lly) ($urx $ury)"]
  }
  return [count_violations_against $shapes $mode]
}

# count signal pin shapes too close to fixed power/ground pins
proc count_pg_pin_violations { mode } {
  set block [ord::get_db_block]
  set shapes {}
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] != "POWER" && [$bterm getSigType] != "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        lappend shapes [list [[$box getTechLayer] getName] [$box xMin] \
          [$box yMin] [$box xMax] [$box yMax] "[$bterm getName] pin"]
      }
    }
  }
  return [count_violations_against $shapes $mode]
}
