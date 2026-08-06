# Helper procs for the boundary PDN tests. A violation is a signal pin shape
# closer to a blocking shape than the required spacing: "overlap" requires
# plain intersection, "spacing" uses the layer default spacing and "table"
# the width-dependent spacing for wide shapes.

proc get_required_spacing { layer pin_box llx lly urx ury mode } {
  if { $mode == "overlap" } {
    return 0
  }
  if { [string is integer -strict $mode] } {
    return $mode
  }
  if { $mode == "table" } {
    set shape_width [expr { min($urx - $llx, $ury - $lly) }]
    set pin_length [expr {
      max([$pin_box xMax] - [$pin_box xMin], [$pin_box yMax] - [$pin_box yMin])
    }]
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

# count signal pin shapes too close to same layer special net wires
proc count_pdn_shape_violations { mode } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" || [$bterm getSigType] == "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        foreach net [$block getNets] {
          if { ![$net isSpecial] } {
            continue
          }
          foreach swire [$net getSWires] {
            foreach sbox [$swire getWires] {
              set slayer [$sbox getTechLayer]
              if { $slayer == "NULL" || $slayer != $layer } {
                continue
              }
              set required [get_required_spacing $layer $box [$sbox xMin] \
                [$sbox yMin] [$sbox xMax] [$sbox yMax] $mode]
              if {
                [is_pin_shape_violation $box [$sbox xMin] [$sbox yMin] \
                  [$sbox xMax] [$sbox yMax] $required]
              } {
                report_pin_violation $bterm $box "[$net getName] wire"
                incr violations
              }
            }
          }
        }
      }
    }
  }
  return $violations
}

# count signal pin shapes on the given layer too close to the given rects
proc count_rect_violations { layer_name rects mode } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" || [$bterm getSigType] == "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        if { [$layer getName] != $layer_name } {
          continue
        }
        foreach rect $rects {
          lassign $rect llx lly urx ury
          set required \
            [get_required_spacing $layer $box $llx $lly $urx $ury $mode]
          if { [is_pin_shape_violation $box $llx $lly $urx $ury $required] } {
            report_pin_violation $bterm $box "shape ($llx $lly) ($urx $ury)"
            incr violations
          }
        }
      }
    }
  }
  return $violations
}
