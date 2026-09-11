# Decision thresholds stay in Tcl, so unordered values must be rejected here.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def
set key [string repeat 0 64]
set claims [make_result_file numeric_options.csv]
set fh [open $claims w]
puts $fh "kind,A_name,B_name,target_bit,skipped_reason"
puts $fh "pair,absent_a,absent_b,0,"
close $fh
check "ordinary threshold rejects wholly missing placement evidence" {
  verify_watermark -placement_claims $claims -min_stages 1
} 0
foreach value { NaN nan Inf -Inf 1e999 -0.1 1.1 nonsense {} } {
  check "invalid tau '$value' cannot grant ownership" {
    catch { verify_watermark -placement_claims $claims -tau $value -min_stages 1 }
  } 1
  check "invalid routing alpha '$value' cannot grant ownership" {
    catch { verify_watermark -routing_key_hex $key -routing_alpha $value -min_stages 1 }
  } 1
}
foreach value { 0 1 } {
  check "alpha excludes boundary $value" {
    catch { verify_watermark -routing_key_hex $key -routing_alpha $value -min_stages 1 }
  } 1
}
foreach value { NaN Inf 0 4 1.5 nonsense {} } {
  check "min_stages must be an integer from one to three ($value)" {
    catch { verify_watermark -placement_claims $claims -min_stages $value }
  } 1
}
foreach value { NaN Inf 0 -1 1.5 2147483648 nonsense {} } {
  check "routing draws must fit a positive integer ($value)" {
    catch { verify_watermark -routing_key_hex $key -routing_permutations $value }
  } 1
}
check "a finite routing threshold rejects unrouted evidence" {
  verify_watermark -routing_key_hex $key -routing_alpha 0.01 -min_stages 1
} 0
check "tau zero is allowed but cannot waive the evidence requirement" {
  verify_watermark -placement_claims $claims -tau 0 -min_stages 1
} 0
check "tau's documented one boundary remains allowed" {
  verify_watermark -placement_claims $claims -tau 1 -min_stages 1
} 0
foreach value { NaN Inf -Inf 1e999 -0.1 0 1.1 nonsense {} } {
  check "invalid routing fraction is rejected ($value)" {
    catch { set_routing_watermark -key_hex $key -fraction $value }
  } 1
  check "invalid reporting quantile is rejected ($value)" {
    catch { report_routing_watermark -p $value }
  } 1
}
# Exercise the public Tcl geometry conversions as well as the direct Python
# API in numeric_options.py. Claims must survive rejected invocations.
foreach command { place_watermark cts_watermark } option { -pair_dist_um -sibling_dist_um } {
  foreach value { 1e20 -1 Inf } {
    check "$command rejects invalid distance $value" {
      catch { $command -key_hex $key -claims_file $claims $option $value }
    } 1
  }
}
set fh [open $claims r]
set text [read $fh]
close $fh
check "bad geometry never opens the claims output" {
  expr {$text eq "kind,A_name,B_name,target_bit,skipped_reason\npair,absent_a,absent_b,0,\n"}
} 1
exit_summary
