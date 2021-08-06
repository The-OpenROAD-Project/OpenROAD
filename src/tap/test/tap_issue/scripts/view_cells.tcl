# layout out all cells in SC_LEF in a grid for viewing

read_lef $::env(TECH_LEF)
read_lef $::env(SC_LEF)

# Create the block
set db [ord::get_db]
set chip [odb::dbChip_create $db]
set block [odb::dbBlock_create $chip all_cells]

# Get all the masters
set masters {}
foreach lib [$db getLibs] {
    foreach master [$lib getMasters] {
        lappend masters $master
    }
}

# Find the number of masters & the max width and height of any master
set max_width 0
set max_height 0
set num_masters 0
foreach master $masters {
    set max_width [expr max($max_width, [$master getWidth])]
    set max_height [expr max($max_height, [$master getHeight])]
    incr num_masters
}

# The steps for laying out the cells
set x_step [expr round(1.5 * $max_width)]
set y_step [expr round(1.5 * $max_height)]

# The number of cells per row
set x_width [expr ceil(sqrt($num_masters * $y_step / $x_step))]

# Make the instance array of masters
set x 0
set y 0
foreach master $masters {
    set inst [odb::dbInst_create $block $master [$master getName]]
    $inst setPlacementStatus PLACED
    $inst setLocation [expr $x * $x_step] [expr $y * $y_step]

    incr x
    if {$x == $x_width} {
        set x 0
        incr y
    }
}
gui::design_created
gui::fit
