# Deterministic macro placement for the MockArray Element grid.
#
# Places ARRAY_ROWS x ARRAY_COLS Element macros (named
# ces_row[r].ces_col[c].ces per MockArray.sv) on a regular grid with
# row 0 at the bottom-left and indices increasing toward the upper right.
# Rows are abutted in X (x_pitch = Element width); a small Y gap is left
# for clock routing.

set block [ord::get_db_block]
set core [$block getCoreArea]

set element [lindex [find_macros] 0]
set bbox [$element getBBox]

set x_pitch [$bbox getDX]
set y_pitch [expr { [$bbox getDY] + 4 / [ord::dbu_to_microns 1] }]

set rows $::env(ARRAY_ROWS)
set cols $::env(ARRAY_COLS)

set x_offset [expr {
  [$core xMin] +
  ([$core dx] - ($cols - 1) * $x_pitch - [$bbox getDX]) / 2
}]
set y_offset [expr {
  [$core yMin] +
  ([$core dy] - ($rows - 1) * $y_pitch - [$bbox getDY]) / 2
}]

for { set r 0 } { $r < $rows } { incr r } {
  for { set c 0 } { $c < $cols } { incr c } {
    # Instance names use backslash-escaped brackets in ODB (matching the
    # Verilog escaped identifier form); use brace-quoted format literal.
    set macro_name [format {ces_row\[%d\].ces_col\[%d\].ces} $r $c]
    set x [expr { $c * $x_pitch + $x_offset }]
    set y [expr { $r * $y_pitch + $y_offset }]
    place_macro -macro_name $macro_name \
      -location [list [expr { [ord::dbu_to_microns 1] * $x }] \
        [expr { [ord::dbu_to_microns 1] * $y }]] \
      -orientation R0
  }
}
