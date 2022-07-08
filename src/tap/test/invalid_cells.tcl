source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def invalid_cells.def

catch { \
  tapcell -distance "25" -tapcell_master "TAPCELL_X1x" -endcap_master "TAPCELL_X1x" \
} error
puts $error
