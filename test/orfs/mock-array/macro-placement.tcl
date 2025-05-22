# Define the variables x and y for the offset

set block [ord::get_db_block]
set core [$block getCoreArea]

set element [lindex [find_macros] 0]
set bbox [$element getBBox]

# Calculate the x and y pitch
set x_pitch [$bbox getDX]
set y_pitch [expr [$bbox getDY] + 4/[ord::dbu_to_microns 1]]

# Define the base location
set x_offset [expr [$core xMin] + ([$core dx] - (7 * $x_pitch) - [$bbox getDX])/2]
set y_offset [expr [$core yMin] + ([$core dy] - (7 * $y_pitch) - [$bbox getDY])/2]

# Loop through the 8x8 array, add the offset, and invoke place_macro
for {set i 0} {$i < 8} {incr i} {
    for {set j 0} {$j < 8} {incr j} {
        set macro_name [format "ces_%d_%d" $i $j]
        set x_location [expr {$j * $x_pitch + $x_offset}]
        set y_location [expr {$i * $y_pitch + $y_offset}]
        place_macro -macro_name $macro_name -location [list [expr [ord::dbu_to_microns 1] * $x_location] [expr [ord::dbu_to_microns 1] * $y_location]] -orientation R0
    }
}
