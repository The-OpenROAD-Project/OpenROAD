# replace_hier_module with modules that have child module instances:
# Outer (one child) -> Outer_new (two levels of children) -> Outer, an
# undone swap, and Outer_new again.  The child modules have instance and
# net names (_1_, _013_) that their parents also have.  Outer_new is copied
# from instance outer2 in the top block, Outer from the child block the
# first swap saved it to.

source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

read_verilog replace_hier_mod7.v
link_design top -hier
create_clock -period 1 clk

set block [ord::get_db_block]

proc module_names { } {
  return [lsort [lmap m [$::block getModules] { $m getName }]]
}

proc net_names { } {
  return [lsort [lmap n [$::block getNets] { $n getName }]]
}

# The nets under instance outer, by their hierarchical names.
proc outer_nets { } {
  return [lsearch -all -inline [net_names] outer/*]
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

proc master_name { modinst_name } {
  return [[[$::block findModInst $modinst_name] getMaster] getName]
}

# The child instance names of a module instance.
proc children { modinst_name } {
  set master [[$::block findModInst $modinst_name] getMaster]
  return [lsort [lmap c [$master getChildren] { $c getName }]]
}

# How many module instances have the module of this one.
proc instances_of_master { modinst_name } {
  set master [[$::block findModInst $modinst_name] getMaster]
  set count 0
  foreach m [$::block getModules] {
    foreach c [$m getChildren] {
      if { [$c getMaster] eq $master } {
        incr count
      }
    }
  }
  return $count
}

# The instance the worst path through r2/D starts at: r1 when the swapped-in
# module's cells are connected to the registers around it.
proc r2_startpoint { } {
  set path [lindex [find_timing_paths -through [get_pins r2/D]] 0]
  set pin [get_full_name [get_property $path startpoint]]
  return [file dirname $pin]
}

set outer_nets_initial {outer/_013_ outer/_014_ outer/inner/_013_}
set outer_nets_new {
  outer/_013_ outer/_014_ outer/_015_ outer/inner/_013_
  outer/mid/_013_ outer/mid/inner/_013_
}

proc check_outer { state master_pattern expected_nets } {
  check "$state: outer is an instance of $master_pattern" {
    string match $master_pattern [master_name outer]
  } 1
  check "$state: net names are unique" {
    expr { [llength [net_names]] == [llength [lsort -unique [net_names]]] }
  } 1
  check "$state: outer's nets are named from outer down" {
    outer_nets
  } [lsort $expected_nets]
  check "$state: outer/_1_ reads r1's output" { pin_net outer/_1_ A } r1q
  check "$state: r2 reads outer's output" { pin_net r2 D } oz
  check "$state: the path to r2 starts at r1" { r2_startpoint } r1
}

proc check_outer_new { state } {
  check_outer $state Outer_new* $::outer_nets_new
  check "$state: outer's children are inner and mid" {
    children outer
  } {inner mid}
  check "$state: outer/mid's child is inner" { children outer/mid } inner
  foreach child {outer/inner outer/mid outer/mid/inner} {
    check "$state: $child has a module of its own" {
      instances_of_master $child
    } 1
  }
  check "$state: outer2 keeps its children" { children outer2 } {inner mid}
  check "$state: Outer and its child's module Inner are gone" {
    lmap m {Outer Inner} { expr { $m in [module_names] } }
  } {0 0}
  check "$state: outer/_1_ drives outer/mid/inner/_1_" {
    pin_net outer/mid/inner/_1_ A
  } outer/_013_
  check "$state: outer/mid's output reaches outer/inner" {
    pin_net outer/inner/_1_ A
  } outer/_014_
  check "$state: outer/inner's output reaches outer/_2_" {
    pin_net outer/_2_ A
  } outer/_015_
}

proc check_outer_initial { state } {
  check_outer $state Outer $::outer_nets_initial
  check "$state: outer's child is inner" { children outer } inner
  check "$state: outer/inner reads outer/_013_" {
    pin_net outer/inner/_1_ A
  } outer/_013_
}

check_outer_initial "before any swap"

set_debug_level ODB replace_design_check_sanity 1

# Swap to Outer_new: its children, two levels deep, come with it.
check "swap to Outer_new" {
  catch { replace_hier_module outer Outer_new }
} 0
check_outer_new "Outer_new"

# Swap back to Outer, from the child block the first swap saved it to.
check "swap back to Outer" {
  catch { replace_hier_module outer Outer }
} 0
check_outer_initial "back to Outer"

# A swap undone restores the design.
set modules_before_undo [module_names]
odb::dbDatabase_beginEco $block
replace_hier_module outer Outer_new
odb::dbDatabase_endEco $block
odb::dbDatabase_undoEco $block
check_outer_initial "undone swap"
check "undone swap: the modules are as before" {
  module_names
} $modules_before_undo

# A second swap to Outer_new works as the first.
check "swap to Outer_new again" {
  catch { replace_hier_module outer Outer_new }
} 0
check_outer_new "Outer_new again"

exit_summary
