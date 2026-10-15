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

proc check_block_prop { block name expected } {
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

proc check_domain_has_top_scope { block domain } {
  set pd [$block findPowerDomain $domain]
  if { $pd == "NULL" } {
    puts "FAIL: $domain not created"
  } elseif { [lsearch -exact [$pd getElements] "."] >= 0 } {
    puts "PASS: $domain element '.' present"
  } else {
    puts "FAIL: $domain elements = [$pd getElements]"
  }
}

# Runs a command that is expected to raise an error.
proc expect_error { cmd } {
  if { [catch { uplevel 1 $cmd }] } {
    puts "PASS: error raised by: $cmd"
  } else {
    puts "FAIL: no error raised by: $cmd"
  }
}

# upf_version 2.1 is the supported version: no warning
check_block_prop $block "__upf_version" "2.1"

# Supply ports: default direction is in; out and inout are accepted
check_block_prop $block "__upf_supply_port:VDD" "in"
check_block_prop $block "__upf_supply_port:VSS" "in"
check_block_prop $block "__upf_supply_port:VOUT" "out"
check_block_prop $block "__upf_supply_port:VBIDIR" "inout"

# Supply nets (value = domains the net is available in). VDD_NET was created
# in PD_D1 and made available in PD_D2 with -reuse.
check_block_prop $block "__upf_supply_net:VDD_NET" "PD_D1 PD_D2"
check_block_prop $block "__upf_supply_net:VSS_NET" "PD_D1"

# Supply-net connections: supply port, design port and hierarchical pin
check_block_prop $block "__upf_supply_net_conn:VDD_NET" "VDD"
check_block_prop $block "__upf_supply_net_conn:VSS_NET" "VSS in1 d1/_0_/VGND"

check_domain_has_top_scope $block PD_TOP

# Repeated -supply: one property per supply set handle
set pd_d1 [$block findPowerDomain PD_D1]
check_block_prop $pd_d1 "__upf_supply:primary" "SS_D1"
check_block_prop $pd_d1 "__upf_supply:default_isolation" ""

# set_isolation -isolation_supply_set / -source / -sink stored on isolation
set iso [$block findIsolation iso_d_1]
if { $iso == "NULL" } {
  puts "FAIL: iso_d_1 not created"
} else {
  check_block_prop $iso "__upf_iso_isolation_supply_set" "SS_D1"
  check_block_prop $iso "__upf_iso_source" "SS_TOP"
  check_block_prop $iso "__upf_iso_sink" "SS_D1"
}

# Connecting a net to multiple ports must accumulate, not overwrite.
# VDD_NET already has VDD; adding {VDD VSS} de-dups VDD and appends VSS.
connect_supply_net VDD_NET -ports {VDD VSS}
check_block_prop $block "__upf_supply_net_conn:VDD_NET" "VDD VSS"

# Unknown ports are rejected with a warning and not recorded
connect_supply_net VDD_NET -ports {NO_SUCH_PORT d1/_0_/NO_SUCH_PIN}
check_block_prop $block "__upf_supply_net_conn:VDD_NET" "VDD VSS"

# connect_supply_net without ports is an error
expect_error { connect_supply_net VDD_NET }
expect_error { connect_supply_net VDD_NET -ports {} }

# Invalid supply port direction
create_supply_port VBAD -direction sideways
if { [odb::dbStringProperty_find $block "__upf_supply_port:VBAD"] == "NULL" } {
  puts "PASS: VBAD not created"
} else {
  puts "FAIL: VBAD created"
}

# -reuse semantics
# re-applying the same -reuse is a no-op
create_supply_net VDD_NET -domain PD_D2 -reuse
check_block_prop $block "__upf_supply_net:VDD_NET" "PD_D1 PD_D2"
# redeclaring without -reuse is rejected and does not change the domains
create_supply_net VDD_NET -domain PD_D2
check_block_prop $block "__upf_supply_net:VDD_NET" "PD_D1 PD_D2"
# -reuse requires -domain
expect_error { create_supply_net VDD_NET -reuse }
# -reuse requires an existing supply net
expect_error { create_supply_net NO_SUCH_NET -domain PD_D2 -reuse }
# -reuse requires the original supply net to have been created with -domain
create_supply_net NODOMAIN_NET
expect_error { create_supply_net NODOMAIN_NET -domain PD_D2 -reuse }
check_block_prop $block "__upf_supply_net:NODOMAIN_NET" ""

# -supply expects {supply_set_handle [supply_set_ref]}
expect_error { create_power_domain PD_BAD_SUPPLY -supply {a b c} }

# -include_scope is deprecated in UPF 2.1 but still mapped onto '.'
create_power_domain PD_OLD -include_scope
check_domain_has_top_scope $block PD_OLD

# Versions other than 2.1 are accepted with a warning
upf_version 2.0
check_block_prop $block "__upf_version" "2.0"
