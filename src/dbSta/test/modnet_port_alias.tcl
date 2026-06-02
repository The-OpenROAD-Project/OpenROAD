# Regression test for the dbReadVerilog deep-descendant modBTerm
# false-attach bug. See modnet_port_alias.v for the trigger pattern.
#
# With the fix in src/dbSta/src/dbReadVerilog.cc, Verilog2db::staToDb
# must resolve the escaped child instance pin to a dbModITerm. check_axioms
# must NOT report ORD-2059, and link_design must NOT abort with ORD-2030
# from the cross-aliased flat net.

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog modnet_port_alias.v
link_design top -hier

sta::check_axioms
