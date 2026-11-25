# replace_hier_module where the new module and the instance it replaces
# overlap in the hierarchy.  A module above the instance would have to
# contain itself, so the swap is refused and nothing changes.  A module
# below the instance is copied from a child block saved before the old
# module, and the modules below it, are deleted.

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

proc child_block_names { } {
  return [lmap b [$::block getChildren] { $b getName }]
}

proc net_names_unique { } {
  expr { [llength [net_names]] == [llength [lsort -unique [net_names]]] }
}

# A module one level below the instance: outer becomes a copy of Inner.
set inner [[master_of outer/inner] getName]
check "outer can become a copy of its child's module" {
  expr { [swap outer [master_of outer/inner]] ne "NULL" }
} 1
check "outer is an instance of a copy of Inner" {
  string match ${inner}* [[master_of outer] getName]
} 1
check "outer has no children" { children outer } {}
check "outer/_1_ reads r1's output" { pin_net outer/_1_ A } r1q
check "outer/_1_ drives outer/_2_" {
  expr { [pin_net outer/_1_ ZN] eq [pin_net outer/_2_ A] }
} 1
check "outer/_2_ drives r2" { pin_net outer/_2_ ZN } oz
check "oz connects outer/_2_ and r2" {
  llength [[$block findNet oz] getITerms]
} 2
check "the path to r2 starts at r1" { startpoint r2 } r1

# A module with a child of its own, one level below the instance.
check "outer2 can become a copy of its child mid's module" {
  expr { [swap outer2 [master_of outer2/mid]] ne "NULL" }
} 1
check "outer2's child is inner" { children outer2 } inner
check "outer2/inner reads r1's output" { pin_net outer2/inner/_1_ A } r1q
check "outer2/_1_ drives r3" { pin_net outer2/_1_ Z } oz2
check "the path to r3 starts at r1" { startpoint r3 } r1

# A module two levels below the instance.
check "outer3 can become a copy of its grandchild's module" {
  expr { [swap outer3 [master_of outer3/mid/inner]] ne "NULL" }
} 1
check "outer3 has no children" { children outer3 } {}
check "outer3/_1_ reads r1's output" { pin_net outer3/_1_ A } r1q
check "outer3/_2_ drives r4" { pin_net outer3/_2_ ZN } oz3
check "the path to r4 starts at r1" { startpoint r4 } r1

check "net names are unique" { net_names_unique } 1
check "no child block is left behind for the copied modules" {
  llength [lsearch -all [child_block_names] *_swap_source]
} 0

exit_summary
