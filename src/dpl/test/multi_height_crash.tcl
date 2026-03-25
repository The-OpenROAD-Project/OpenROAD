# improve_placement crashes with bad_optional_access when a design
# contains multi-height BLOCK macros (e.g. SRAMs) alongside standard
# cells. getSiteOrientation() returns nullopt for grid positions where
# the cell's site isn't in the row map, and 5 call sites use .value()
# without checking.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef multi_height_crash_sram.lef
read_def multi_height_crash.def
improve_placement
