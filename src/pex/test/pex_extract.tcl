# Extraction of the sky130hd gcd excerpt of the fsv tests with the small
# model, its SPEF read back by OpenSTA.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130hd_tt.lib
read_def ../../fsv/test/field_solver_gcd.def
read_pex_model -itf ../../fsv/test/sky130.itf sky130_coarse.json
extract_pex

set spef [make_result_file pex_extract.spef]
write_pex_spef $spef
set file [open $spef]
set nets [regexp -all -line {^\*D_NET} [read $file]]
close $file
check "every extracted net is written" { set nets } 122

read_spef $spef
report_net -digits 3 _042_
report_net -digits 3 req_msg[10]

# Two nets against the field solver, at a loose tolerance.
compare_pex_fsv -nets {_042_ req_msg[10]} -halo 4 -tol 0.02

# One net, and no coupling kept below 1 fF.
set one [make_result_file pex_extract_one.spef]
write_pex_spef -nets _042_ $one
set file [open $one]
set nets [regexp -all -line {^\*D_NET} [read $file]]
close $file
check "-nets writes those nets" { set nets } 1
extract_pex -coupling_threshold 1 -coupling_ratio 1

exit_summary
