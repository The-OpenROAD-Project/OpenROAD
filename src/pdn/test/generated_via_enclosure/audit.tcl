# Inspect actual OpenDB via rectangles; this is independent of PDN's fit logic.
# These fixtures have no cut-layer enclosure overrides. Do not use this
# default-only comparison as a general process DRC or override-rule checker.
proc audit_generated_vias {block} {
  set count 0
  set invalid 0
  set negative 0
  set geometries [dict create]
  set out [open via_geometry.tsv w]
  puts $out "via\tlayer\tcut_bbox\tmetal_bbox\tmargins\trule_minima\tpass"
  foreach via [$block getVias] {
    set rule [$via getViaGenerateRule]
    if {$rule eq "NULL" || $rule eq ""} { continue }
    set layer_rects [dict create]
    set cuts [list]
    foreach box [$via getBoxes] {
      set layer [$box getTechLayer]
      set rect [list [$box xMin] [$box yMin] [$box xMax] [$box yMax]]
      if {[$layer getType] eq "CUT"} {
        lappend cuts $rect
      } else {
        dict lappend layer_rects [$layer getName] $rect
      }
    }
    if {[llength $cuts]==0} { error "Generated via has no cut geometry" }
    set cut [lindex $cuts 0]
    foreach rect [lrange $cuts 1 end] {
      set cut [list [expr {min([lindex $cut 0],[lindex $rect 0])}] \
                    [expr {min([lindex $cut 1],[lindex $rect 1])}] \
                    [expr {max([lindex $cut 2],[lindex $rect 2])}] \
                    [expr {max([lindex $cut 3],[lindex $rect 3])}]]
    }
    set via_valid 1
    for {set k 0} {$k < [$rule getViaLayerRuleCount]} {incr k} {
      set lr [$rule getViaLayerRule $k]
      if {![$lr hasEnclosure]} { continue }
      set name [[$lr getLayer] getName]
      set rects [dict get $layer_rects $name]
      if {[llength $rects]!=1} { error "Unsupported multi-rectangle via landing" }
      set metal [lindex $rects 0]
      set margins [list [expr {[lindex $cut 0]-[lindex $metal 0]}] \
                        [expr {[lindex $cut 1]-[lindex $metal 1]}] \
                        [expr {[lindex $metal 2]-[lindex $cut 2]}] \
                        [expr {[lindex $metal 3]-[lindex $cut 3]}]]
      set x [expr {min([lindex $margins 0],[lindex $margins 2])}]
      set y [expr {min([lindex $margins 1],[lindex $margins 3])}]
      lassign [$lr getEnclosure] a b
      set valid [expr {($x >= $a && $y >= $b) || ($x >= $b && $y >= $a)}]
      if {!$valid} {set via_valid 0}
      puts $out "[$via getName]\t$name\t$cut\t$metal\t$margins\t$a $b\t$valid"
    }
    dict set geometries [$via getName] $via_valid
  }
  close $out
  set out [open via_enclosures.tsv w]
  puts $out "net\tvia\tbottom_x\tbottom_y\ttop_x\ttop_y\tgeometry_valid"
  foreach net [$block getNets] {
    foreach wire [$net getSWires] {
      foreach box [$wire getWires] {
        if {![$box isVia]} { continue }
        set via [$box getBlockVia]
        if {$via eq "NULL" || $via eq ""} { continue }
        set name [$via getName]
        if {![dict exists $geometries $name]} { continue }
        incr count
        set params [$via getViaParams]
        set enc [list [$params getXBottomEnclosure] [$params getYBottomEnclosure] \
                      [$params getXTopEnclosure] [$params getYTopEnclosure]]
        set valid [dict get $geometries $name]
        puts $out "[$net getName]\t$name\t[join $enc \t]\t$valid"
        if {!$valid} { incr invalid }
        foreach value $enc { if {$value < 0} { incr negative; break } }
      }
    }
  }
  close $out
  return [list $count $negative $invalid]
}
