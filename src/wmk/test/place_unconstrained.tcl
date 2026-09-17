# Liberty without constraints must not silently enable the placement guard.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
set_wire_rc -signal -layer metal3
set claims [make_result_file place_unconstrained.csv]
set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
tee -variable output [list place_watermark -key_hex $key -claims_file $claims \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 64]
check "the unavailable timing guard is reported" {
  string match {*WMK-0059*} $output
} 1
exit_summary
