# A successful embed with fewer than two leaves must publish empty claims,
# including when the requested path holds evidence from an earlier run.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog cts_modes.v
link_design cts_modes

set block [ord::get_db_block]
set key [string repeat 0 64]
set before [make_result_file cts_empty_before.v]
set after [make_result_file cts_empty_after.v]
write_verilog $before

foreach leaves { 0 1 } {
  # Leaf eligibility requires a clock-typed output net with sequential sinks.
  [$block findNet clock_a] setSigType [expr { $leaves == 0 ? "SIGNAL" : "CLOCK" }]
  [$block findNet clock_b] setSigType SIGNAL
  foreach existing { 1 0 } {
    set claims [make_result_file cts_empty_${leaves}_${existing}.csv]
    file delete -force $claims
    if { $existing } {
      # A claim the key would have written, so it is checkable.
      set target [wmk::cts_target_lcb_cmd $key leaf_a leaf_b]
      set other [expr { $target eq "leaf_a" ? "leaf_b" : "leaf_a" }]
      set stream [open $claims w]
      puts $stream "target_lcb,other_lcb,target_bit,skipped_reason"
      puts $stream "$target,$other,[wmk::cts_target_bit_cmd $key leaf_a leaf_b],"
      close $stream
      check "old claims are checkable with $leaves leaves" {
        lassign [wmk::verify_cts_claims_cmd $key $claims] count held probability
        set count
      } 1
    }
    check "$leaves leaves produce no pairs (existing=$existing)" {
      cts_watermark -key_hex $key -claims_file $claims
    } 0
    check "empty claims are published (leaves=$leaves, existing=$existing)" {
      file isfile $claims
    } 1
    check "empty claims parse without checkable evidence" {
      wmk::verify_cts_watermark_cmd $key $claims
    } -1.0
    check "empty claims cannot prove ownership" {
      verify_watermark -cts_claims $claims -cts_key_hex $key -min_stages 1
    } 0
  }

  set directory [make_result_file cts_empty_directory_$leaves]
  file mkdir $directory
  check "invalid output is rejected even with $leaves leaves" {
    catch { cts_watermark -key_hex $key -claims_file $directory }
  } 1
  check "rejected output preserves the directory" { file isdirectory $directory } 1
  file delete $directory
}
write_verilog $after
check "empty embeddings preserve connectivity" { diff_files $before $after } 0
exit_summary
