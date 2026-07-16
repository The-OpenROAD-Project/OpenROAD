# Exercise the sta::sta_to_db_* SWIG bindings on a hierarchical design.
#
# Coverage:
#   * leaf instance (b1/u1)         -> dbInst , mod_inst NULL
#   * hierarchical instance (b1)    -> dbInst NULL, dbModInst
#   * leaf pin (b1/u1/A)            -> dbITerm, mod_pin NULL
#   * hierarchical inst pin (b1/clk)-> dbITerm NULL, dbModITerm
#   * top-level port (clk1)         -> dbBTerm, dbModBTerm NULL
#   * top-level pin (clk1)          -> dbBTerm via sta_to_db_pin? (see ok)
#   * dbNet only (top flat net)     -> dbNet , mod_net NULL
#   * dbModNet only (b1.u1out)      -> dbNet NULL, dbModNet
#   * sta_pin_to_port for a moditerm should return a Port wrapping the
#     dbModBTerm of the corresponding child module.
source "helpers.tcl"

read_lef example1.lef
read_liberty example1_typ.lib
read_verilog hier3.v
link_design top -hier

proc check { label obj } {
  if { $obj == "NULL" } {
    puts "$label: NULL"
    return
  }
  set name [$obj getName]
  if { [catch { set hier [$obj getHierarchicalName] }] } {
    puts "$label: $name"
  } else {
    puts "$label: $name (hier: $hier)"
  }
}

proc check_port { label port } {
  if { $port == "NULL" } {
    puts "$label: <no port>"
  } else {
    puts "$label: [get_property $port full_name]"
  }
}

# --- leaf instance b1/u1 ---
puts "# leaf instance b1/u1"
set inst [get_cells b1/u1]
check "  sta_to_db_inst" [sta::sta_to_db_inst $inst]
check "  sta_to_db_mod_inst" [sta::sta_to_db_mod_inst $inst]

# --- leaf pin b1/u1/A ---
puts "# leaf pin b1/u1/A"
set pin [get_pin b1/u1/A]
check "  sta_to_db_pin" [sta::sta_to_db_pin $pin]
check "  sta_to_db_mod_pin" [sta::sta_to_db_mod_pin $pin]
check_port "  sta_pin_to_port" [sta::sta_pin_to_port $pin]

# --- hierarchical instance b1 ---
puts "# hierarchical instance b1"
set hinst [get_cells b1]
check "  sta_to_db_inst" [sta::sta_to_db_inst $hinst]
check "  sta_to_db_mod_inst" [sta::sta_to_db_mod_inst $hinst]

# --- hier-instance pin b1/clk (should be a dbModITerm) ---
puts "# hier-instance pin b1/clk"
set hpin [get_pin b1/clk]
check "  sta_to_db_pin" [sta::sta_to_db_pin $hpin]
check "  sta_to_db_mod_pin" [sta::sta_to_db_mod_pin $hpin]
# sta_pin_to_port returns a Port wrapping the child-module modBTerm.
set hport [sta::sta_pin_to_port $hpin]
check_port "  sta_pin_to_port" $hport
if { $hport != "NULL" } {
  check "  sta_to_db_port (from moditerm)" [sta::sta_to_db_port $hport]
  check "  sta_to_db_mod_port (from moditerm)" [sta::sta_to_db_mod_port $hport]
}

# --- top-level port clk1 ---
puts "# top-level port clk1"
set port [lindex [get_ports clk1] 0]
check "  sta_to_db_port" [sta::sta_to_db_port $port]
check "  sta_to_db_mod_port" [sta::sta_to_db_mod_port $port]

# --- top-level (flat) dbNet clk1 ---
puts "# top-level net clk1"
set tnet [get_net clk1]
check "  sta_to_db_net" [sta::sta_to_db_net $tnet]
check "  sta_to_db_mod_net" [sta::sta_to_db_mod_net $tnet]

# --- flat-name lookup of an internal hier net ---
# get_net resolves to the flat dbNet view (when a flat dbNet exists),
# so sta_to_db_net resolves and sta_to_db_mod_net returns NULL.
puts "# flat-name net b1/u1out"
set hnet [get_net b1/u1out]
check "  sta_to_db_net" [sta::sta_to_db_net $hnet]
check "  sta_to_db_mod_net" [sta::sta_to_db_mod_net $hnet]

# --- modnet-wrapped Net via Pin::net() ---
# In hier mode, network->net(pin) prefers the dbModNet over the flat
# dbNet, so the returned Net is dispatched through the dbModNet branch
# of staToDb. Exercises sta_to_db_mod_net with a non-NULL result.
puts "# modnet-wrapped Net via \[\$pin net\] (b2/u3/Z)"
set leaf_pin [get_pin b2/u3/Z]
set mod_net [$leaf_pin net]
check "  sta_to_db_net" [sta::sta_to_db_net $mod_net]
check "  sta_to_db_mod_net" [sta::sta_to_db_mod_net $mod_net]

puts "# modnet-wrapped Net via \[\$pin net\] (b1/clk hier-pin)"
set hier_clk_pin [get_pin b1/clk]
set hier_clk_net [$hier_clk_pin net]
check "  sta_to_db_net" [sta::sta_to_db_net $hier_clk_net]
check "  sta_to_db_mod_net" [sta::sta_to_db_mod_net $hier_clk_net]
