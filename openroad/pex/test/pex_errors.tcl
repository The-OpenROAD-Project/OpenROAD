# Argument and state errors of the pex commands.
source "helpers.tcl"

check_error "extraction needs a model" { extract_pex } PEX-0016
check_error "a comparison needs a model" { compare_pex_fsv -sample 2 } PEX-0024
check_error "-nets or -sample" { compare_pex_fsv } PEX-0036
check_error "not both -nets and -sample" {
  compare_pex_fsv -nets a -sample 2
} PEX-0034
check_error "-sample is a count" { compare_pex_fsv -sample 0 } PEX-0035
check_error "-itf is required" { read_pex_model sky130_coarse.json } PEX-0030
check_error "a model of another stack" {
  read_pex_model -itf ../../fsv/test/sky130.itf other_stack.json
} PEX-0014
check_error "-max_res is not negative" { extract_pex -max_res -1 } PEX-0032
check_error "-coupling_ratio is a fraction" {
  extract_pex -coupling_ratio 1.5
} PEX-0041
check_error "-tol is positive" {
  build_pex_model -itf ../../fsv/test/sky130.itf -out x.json -tol 0
} PEX-0031
check_error "a layer the stack does not have" {
  build_pex_model -itf ../../fsv/test/sky130.itf -out x.json -layers met9
} PEX-0004
check_error "the routing layers come from a technology" {
  build_pex_model -itf ../../fsv/test/sky130.itf -out x.json
} PEX-0003

# A model of format version 2 does not record its stack.
read_pex_model -itf ../../fsv/test/sky130.itf version_two.json
read_pex_model -itf ../../fsv/test/sky130.itf sky130_coarse.json
check_error "extraction needs a design" { extract_pex } PEX-0001

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def ../../fsv/test/field_solver_gcd.def
extract_pex
check_error "an unknown net" {
  write_pex_spef -nets no_such_net [make_result_file pex_errors.spef]
} PEX-0020
check_error "a net without parasitics" {
  write_pex_spef -nets VDD [make_result_file pex_errors.spef]
} PEX-0021
check_error "an unknown net to compare" {
  compare_pex_fsv -nets no_such_net
} PEX-0025
check_error "a net without parasitics to compare" {
  compare_pex_fsv -nets VDD
} PEX-0027

exit_summary
