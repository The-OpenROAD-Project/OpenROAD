# Ownership is decided by extraction rate, so a claim that no longer holds does
# not by itself sink the verdict, and a claim the embedder skipped is not
# counted at all.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

# The key wm_place_claims.csv was written with.
set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee

# Five claims clear the extraction threshold but provide too little evidence.
puts "verdict [verify_watermark -placement_claims wm_place_claims.csv \
  -placement_key_hex $key -min_stages 1]"

# The same evidence is not enough once the bar is raised past it.
set strict [verify_watermark -placement_claims wm_place_claims.csv \
  -placement_key_hex $key -tau 0.9 -min_stages 1]
puts "strict verdict $strict"

# A claim file that does not say which bit it committed to is refused rather
# than scored.  Reading a missing bit as zero would measure the design against
# a target nobody committed to, and still report an extraction rate for it.
set damaged [make_result_file "wm_place_damaged.csv"]
set fh [open $damaged w]
puts $fh "kind,id,A_name,B_name,target_bit,skipped_reason"
puts $fh "pair,_276_|_277_,_276_,_277_,,"
close $fh
catch {
  tee -quiet -variable diagnostic \
    [list verify_watermark -placement_claims $damaged -placement_key_hex $key \
      -min_stages 1]
} message
puts -nonewline [string map [list $damaged [file tail $damaged]] $diagnostic]
puts "refused: $message"
