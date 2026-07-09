source "helpers.tcl"
# Multi-patterned tech: met1 has MASK 2 (double patterned), min spacing 0.14um.
read_lef sky130hd/sky130hd_multi_patterned.tlef
read_def mask_drc.def

# Flag is OFF by default: check_mask_drc must refuse to run. Verify the gate.
set err [catch { drt::check_mask_drc -output_file /dev/null } msg]
if { $err == 0 } {
  error "check_mask_drc ran with mask awareness disabled (gate failed)"
}
puts "GATE_OK: disabled by default"

# Enable mask awareness, then run the audit.
set_mask_aware_routing -enable
set drc_file [make_result_file mask_drc.drc]
set viols [drt::check_mask_drc -output_file $drc_file]
puts "VIOLATIONS: $viols"
diff_files $drc_file mask_drc.drcok
