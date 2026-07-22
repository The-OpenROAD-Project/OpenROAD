source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_lef invalid_master_class.lef
read_def boundary_macros.def

catch {
  tapcell -distance 20 -tapcell_master TAPCELL_X1 \
    -cnrcap_nwin_master CORNER_TOPLEFT_LEGACY
} error
puts $error

catch {
  place_endcaps -corner CORNER_TOPLEFT_LEGACY
} error
puts $error

catch {
  place_tapcells -master CORNER_TOPLEFT_LEGACY -distance 20
} error
puts $error
