# replace_hier_module where the new module and the instance it replaces
# overlap in the hierarchy.  A module above the instance would have to
# contain itself, so the swap is refused and nothing changes.

source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

read_verilog replace_hier_mod8.v
link_design top -hier
create_clock -period 1 clk

set block [ord::get_db_block]
set_debug_level ODB replace_design_check_sanity 1

proc module_names { } {
  return [lsort [lmap m [$::block getModules] { $m getName }]]
}

proc net_names { } {
  return [lsort [lmap n [$::block getNets] { $n getName }]]
}

# The net on an instance's pin, by hierarchical instance name.
proc pin_net { inst_name pin } {
  set inst [$::block findInst $inst_name]
  if { $inst eq "NULL" } {
    return "no instance"
  }
  set net [[$inst findITerm $pin] getNet]
  if { $net eq "NULL" } {
    return "no net"
  }
  return [$net getName]
}

proc master_of { modinst_name } {
  return [[$::block findModInst $modinst_name] getMaster]
}

# The child instance names of a module instance.
proc children { modinst_name } {
  set master [master_of $modinst_name]
  return [lsort [lmap c [$master getChildren] { $c getName }]]
}

# The instance the worst path through a register's D pin starts at.
proc startpoint { reg } {
  set path [lindex [find_timing_paths -through [get_pins $reg/D]] 0]
  set pin [get_full_name [get_property $path startpoint]]
  return [file dirname $pin]
}

# Swap through the odb API, which returns NULL when the swap is refused.
proc swap { modinst_name module } {
  return [[$::block findModInst $modinst_name] swapMaster $module]
}

# A module above the instance it would replace.
set modules_before [module_names]
set nets_before [net_names]
check "outer/inner cannot become a copy of its parent's module" {
  swap outer/inner [master_of outer]
} NULL
check "outer2/mid/inner cannot become a copy of its grandparent's module" {
  swap outer2/mid/inner [master_of outer2]
} NULL
check "refused swaps leave the modules as they were" {
  module_names
} $modules_before
check "refused swaps leave the nets as they were" { net_names } $nets_before
check "outer keeps its child" { children outer } inner
check "the path to r2 still starts at r1" { startpoint r2 } r1

exit_summary
