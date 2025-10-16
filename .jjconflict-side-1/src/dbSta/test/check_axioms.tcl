# Test for sanity checker APIs (sta::check_axioms)
source "helpers.tcl"

# Setup
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog check_axioms.v
link_design top -hier

# Run the checks and capture output.
# The 'catch' command is used because sta::check_axioms will exit with an
# error, and we want to continue to check all messages.
set status [catch { sta::check_axioms } msg]

# check_axioms cannot detect unused_module.
# - It is because odb does not keep unused modules in its database.

# Check for expected messages.
#[WARNING ORD-2039] SanityCheck: Net 'out_unconnected' has less than
# 2 connections (# of ITerms = 0, # of BTerms = 1).
#[WARNING ORD-2039] SanityCheck: Net 'single_conn_wire' has less than
# 2 connections (# of ITerms = 0, # of BTerms = 0).
#[WARNING ORD-2039] SanityCheck: Net 'w2' has less than 2 connections
# (# of ITerms = 0, # of BTerms = 1).
#[WARNING ORD-2038] SanityCheck: Module 'i_empty' has no instances.
#[WARNING ORD-2038] SanityCheck: Module 'u1' has no instances.
#[WARNING ORD-2038] SanityCheck: Module 'u2' has no instances.
