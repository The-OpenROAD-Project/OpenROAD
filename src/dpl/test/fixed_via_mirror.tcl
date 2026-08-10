# Reject an HPWL-improving mirror whose physical OBS conflicts with a supply
# via.  The PDN-aware placement call establishes the mode used by the
# subsequent standalone mirroring operation.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef fixed_via_mirror.lef
read_def fixed_via_mirror.def

detailed_placement -pdn_aware
optimize_mirroring
check_placement
check "orientation" {[[ord::get_db_block] findInst mirror] getOrient} R0
exit_summary
