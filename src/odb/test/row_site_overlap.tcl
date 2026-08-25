# Regression test: the DEF reader must reject rows whose site spacing (STEP)
# is smaller than the site width, because they declare overlapping sites.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
catch { read_def row_site_overlap.def } msg
puts $msg
