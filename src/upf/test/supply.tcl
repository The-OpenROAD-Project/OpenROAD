source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog data/isolation/mpd_top.v
link_design mpd_top

read_upf -file data/isolation/mpd_top_supply.upf

# Inspect the resulting odb state. Supply ports/nets and the UPF version are
# stored as block string-properties (no dedicated odb object exists). The
# isolation supply associations are stored on the dbIsolation object.
set block [[[ord::get_db] getChip] getBlock]

proc check_block_prop {block name expected} {
  set prop [odb::dbStringProperty_find $block $name]
  if { $prop == "NULL" } {
    puts "FAIL: missing property $name"
    return
  }
  set val [$prop getValue]
  if { $val eq $expected } {
    puts "PASS: $name = '$val'"
  } else {
    puts "FAIL: $name = '$val' (expected '$expected')"
  }
}

# upf_version
check_block_prop $block "__upf_version" "2.0"

# Supply ports
check_block_prop $block "__upf_supply_port:VDD" "in"
check_block_prop $block "__upf_supply_port:VSS" "in"

# Supply nets (value = owning domain)
check_block_prop $block "__upf_supply_net:VDD_NET" "PD_D1"
check_block_prop $block "__upf_supply_net:VSS_NET" "PD_D1"

# Supply-net connections (value = port)
check_block_prop $block "__upf_supply_net_conn:VDD_NET" "VDD"
check_block_prop $block "__upf_supply_net_conn:VSS_NET" "VSS"

# create_power_domain -include_scope -> top domain owns '.'
set pd_top [$block findPowerDomain PD_TOP]
if { $pd_top == "NULL" } {
  puts "FAIL: PD_TOP not created"
} else {
  set els [$pd_top getElements]
  if { [lsearch $els "."] >= 0 } {
    puts "PASS: PD_TOP -include_scope element '.' present"
  } else {
    puts "FAIL: PD_TOP elements = $els"
  }
}

# create_power_domain -supply stored on domain
set pd_d1 [$block findPowerDomain PD_D1]
check_block_prop $pd_d1 "__upf_supply" "primary VDD_NET"

# set_isolation -isolation_supply / -source / -sink stored on isolation
set iso [$block findIsolation iso_d_1]
if { $iso == "NULL" } {
  puts "FAIL: iso_d_1 not created"
} else {
  check_block_prop $iso "__upf_iso_isolation_supply" "VDD_NET"
  check_block_prop $iso "__upf_iso_source" "VDD_NET"
  check_block_prop $iso "__upf_iso_sink" "VSS_NET"
}
