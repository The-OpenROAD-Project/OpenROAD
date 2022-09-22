# read_verilog missing liberty cell, missing lef and liberty cell
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
# BUF_X10
read_lef read_verilog6.lef
read_liberty Nangate45/Nangate45_typ.lib
# uses BUF_X10 (no liberty) and AND2_X10 (no lef or liberty)
read_verilog read_verilog6.v
link_design top
