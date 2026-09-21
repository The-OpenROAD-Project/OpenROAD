# report_gcell_density for the gcell holding a point
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def simple01.def
detailed_placement

# Global routing would leave this behind; building it here directly keeps
# the test inside dpl. The die is 10um x 10um, so this is a 4x4 grid of
# 2.5um gcells.
set block [ord::get_db_block]
set gcell_grid [odb::dbGCellGrid_create $block]
$gcell_grid addGridPatternX 0 4 5000
$gcell_grid addGridPatternY 0 4 5000

# The gcell the instance fills.
report_gcell_density 2.0 2.0
# The one next to it, which the instance only reaches into.
report_gcell_density 3.0 3.0
# A gcell inside the rows but empty.
report_gcell_density 6.0 6.0
# A gcell past the core, which has no placement area to report on.
report_gcell_density 9.0 9.0

# Widening the window around the filled gcell dilutes it. Radius 1 is the
# 3x3 neighbourhood, radius 2 the 5x5 one, which here is the whole core.
report_gcell_density 2.0 2.0 -radius 1
report_gcell_density 2.0 2.0 -radius 2

# The core stops at 7.98 x 7.0, so the window is clipped there rather than
# running out to the 10 x 10 die.
report_gcell_density 6.0 6.0 -radius 1
# A radius of 0 is the default, the single gcell.
report_gcell_density 9.0 9.0 -radius 0

catch { report_gcell_density 20.0 20.0 } error
puts $error
catch { report_gcell_density 2.0 2.0 -radius -1 } error
puts $error
