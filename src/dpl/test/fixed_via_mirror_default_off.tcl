# Standalone mirroring does not require placement rows when PDN-aware detailed
# placement has not been requested.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef fixed_via_mirror.lef
read_def fixed_via_mirror_default_off.def

optimize_mirroring

check "standalone mirror orientation" {
  [[ord::get_db_block] findInst mirror] getOrient
} MY
exit_summary
