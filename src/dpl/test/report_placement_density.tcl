# report_placement_density for an arbitrary rectangle
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def simple01.def
detailed_placement

# No gcell grid needed, unlike report_gcell_density.
# The core runs from 1.9 x 1.4 to 7.98 x 7.0 and holds one instance.

# The whole core.
report_placement_density 1.9 1.4 7.98 7.0
# A rectangle around the placed instance.
report_placement_density 1.9 1.4 3.0 2.8
# An empty stretch of rows.
report_placement_density 5.0 5.0 7.0 7.0
# Opposite corners the other way round name the same rectangle.
report_placement_density 7.0 7.0 5.0 5.0
# Off the rows entirely, so there is no site to measure against.
report_placement_density 8.5 8.5 9.5 9.5

catch { report_placement_density 2.0 2.0 2.0 4.0 } error
puts $error
catch { report_placement_density 1.0 2.0 3.0 } error
puts $error
catch { report_placement_density 1.0 2.0 3.0 abc } error
puts $error
