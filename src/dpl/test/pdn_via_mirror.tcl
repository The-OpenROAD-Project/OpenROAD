# Reject an HPWL-improving mirror whose physical OBS conflicts with a supply
# via.  The PDN-aware placement call establishes the mode used by the
# subsequent standalone mirroring operation.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef pdn_via_mirror.lef
read_def pdn_via_mirror.def

# The default mode accepts the HPWL-improving mirror.
detailed_placement
optimize_mirroring
check_placement
set off_def [make_result_file pdn_via_mirror.off.def]
write_def $off_def
check "default-mode placement DEF" {diff_files pdn_via_mirror.off.defok $off_def} 0

# Restore the input bounding-box location before enabling physical conflicts.
set inst [[ord::get_db_block] findInst mirror]
$inst setOrient R0
$inst setLocation 3800 5600
detailed_placement -pdn_aware
optimize_mirroring
check_placement
set def_file [make_result_file pdn_via_mirror.def]
write_def $def_file
check "placement DEF" {diff_files pdn_via_mirror.defok $def_file} 0
exit_summary
