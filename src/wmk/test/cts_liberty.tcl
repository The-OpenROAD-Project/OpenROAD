# CTS embedding and verification require Liberty to classify clock pins.
source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
set claims [make_result_file cts_liberty.csv]
set fh [open $claims w]
puts $fh "pair_idx,pair_key,target_lcb,other_lcb,target_bit,final_bit,skipped_reason"
puts $fh "0,a+b,a,b,0,0,"
close $fh
set key 0000000000000000000000000000000000000000000000000000000000000000
set failed [catch { cts_watermark -key_hex $key -claims_file $claims } message]
check "embedding requires Liberty" { set failed } 1
check "embedding explains the missing dependency" { string match {*WMK-0107*} $message } 1
set failed [catch { verify_watermark -cts_claims $claims -min_stages 1 } message]
check "verification requires Liberty" { set failed } 1
check "verification explains the missing dependency" { string match {*WMK-0108*} $message } 1
exit_summary
