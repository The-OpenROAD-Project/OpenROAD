source "helpers.tcl"
# Regression test for a segfault in dpl::Grid::markHopeless().
#
# A row whose site spacing (STEP) is smaller than the site width declares
# overlapping sites.  This makes the row's site count exceed the number of
# sites the detailed-placement grid derives from the core area, which used to
# overflow the pixel grid and crash with Signal 11.  DPL now reports a
# controlled error instead.
read_lef Nangate45/Nangate45.lef
read_def row_site_mismatch.def
catch { detailed_placement } msg
puts $msg
