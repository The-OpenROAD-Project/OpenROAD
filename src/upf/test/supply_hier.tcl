# connect_supply_net -ports with hierarchical module-instance ports, which
# only exist when the design is linked hierarchically.
source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog data/isolation/mpd_top.v
link_design -hier mpd_top

set block [[[ord::get_db] getChip] getBlock]

create_power_domain PD_TOP -elements {.}
create_supply_net VDD_NET -domain PD_TOP

# d1/in1 is a port of hierarchical instance d1, in2 is a design port
connect_supply_net VDD_NET -ports {d1/in1 in2}
# d1/no_such_inst/in1 must not resolve to d1/in1
connect_supply_net VDD_NET -ports {d1/no_such_inst/in1 d1/no_such_port}

set prop [odb::dbStringProperty_find $block "__upf_supply_net_conn:VDD_NET"]
if { $prop == "NULL" } {
  puts "FAIL: missing connections for VDD_NET"
} elseif { [$prop getValue] eq "d1/in1 in2" } {
  puts "PASS: VDD_NET connected to '[$prop getValue]'"
} else {
  puts "FAIL: VDD_NET connected to '[$prop getValue]'"
}
