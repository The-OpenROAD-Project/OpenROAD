source $::env(SCRIPTS_DIR)/load.tcl

load_design 2_floorplan.odb 2_floorplan.sdc

set rows $::env(ARRAY_ROWS)
set cols $::env(ARRAY_COLS)
set expected [expr { $rows * $cols }]

set block [ord::get_db_block]

set elements [list]
foreach inst [$block getInsts] {
  set master [$inst getMaster]
  if { ![$master isBlock] } {
    continue
  }
  if { [$master getName] ne "Element" } {
    continue
  }
  lappend elements $inst
}

set count [llength $elements]
if { $count != $expected } {
  puts "ERROR: expected $expected Element macros (${rows}x${cols}), found $count"
  foreach inst $elements {
    puts "  [$inst getName]"
  }
  exit 1
}

# Collect (xMin, yMin, orient, name, width, height) from oriented bboxes.
set entries [list]
set first_orient ""
set macro_width ""
set macro_height ""
foreach inst $elements {
  set bbox [$inst getBBox]
  set x [$bbox xMin]
  set y [$bbox yMin]
  set w [$bbox getDX]
  set h [$bbox getDY]
  set orient [$inst getOrient]
  if { $first_orient eq "" } {
    set first_orient $orient
    set macro_width $w
    set macro_height $h
  } elseif { $orient ne $first_orient } {
    puts "ERROR: Element [$inst getName] has orientation $orient,\
      expected $first_orient (all macros must share orientation)"
    exit 1
  }
  lappend entries [list $x $y $orient [$inst getName] $w $h]
}

# Group by row (unique yMin). Row 0 = minimum Y = bottom.
set rows_map [dict create]
foreach e $entries {
  set y [lindex $e 1]
  dict lappend rows_map $y $e
}

set row_ys [lsort -integer [dict keys $rows_map]]
if { [llength $row_ys] != $rows } {
  puts "ERROR: expected $rows distinct row Y positions, found [llength $row_ys]: $row_ys"
  foreach y $row_ys {
    puts "  y=$y count=[llength [dict get $rows_map $y]]"
  }
  exit 1
}

# Each row must have exactly `cols` macros; capture X positions per row.
set row_x_lists [list]
foreach y $row_ys {
  set row_entries [dict get $rows_map $y]
  if { [llength $row_entries] != $cols } {
    puts "ERROR: row at y=$y has [llength $row_entries] macros, expected $cols"
    foreach e $row_entries {
      puts "  [lindex $e 3] at ([lindex $e 0], [lindex $e 1])"
    }
    exit 1
  }
  set xs [list]
  foreach e [lsort -integer -index 0 $row_entries] {
    lappend xs [lindex $e 0]
  }
  lappend row_x_lists $xs
}

# Columns must align across rows.
set ref_xs [lindex $row_x_lists 0]
for { set r 1 } { $r < $rows } { incr r } {
  set xs [lindex $row_x_lists $r]
  if { $xs ne $ref_xs } {
    puts "ERROR: column X positions differ between row 0 and row $r (y=[lindex $row_ys $r])"
    puts "  row 0: $ref_xs"
    puts "  row $r: $xs"
    exit 1
  }
}

# Abutment: consecutive column X values must differ by exactly macro_width.
for { set c 1 } { $c < $cols } { incr c } {
  set dx [expr { [lindex $ref_xs $c] - [lindex $ref_xs [expr { $c - 1 }]] }]
  if { $dx != $macro_width } {
    puts "ERROR: columns [expr { $c - 1 }] and $c not abutted: dx=$dx, macro_width=$macro_width"
    puts "  column X positions: $ref_xs"
    exit 1
  }
}

puts "OK: ${rows}x${cols} Element macros in grid"
puts "  orientation: $first_orient"
puts "  macro size (DBU): ${macro_width} x ${macro_height}"
puts "  row Y (bottom to top): $row_ys"
puts "  col X (left to right): $ref_xs"

set f [open $::env(OUTPUT) w]
puts $f "OK"
close $f
