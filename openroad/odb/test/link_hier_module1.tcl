# link_hier_module: a module read after the design was loaded is linked
# into it, so replace_hier_module can switch an instance to it.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog replace_hier_mod1.v
link_design top -hier

proc expect_error { what script pattern } {
  if { ![catch { uplevel 1 $script } message] } {
    error "$what: no error"
  }
  if { ![string match $pattern $message] } {
    error "$what: expected $pattern, got $message"
  }
  puts "$what: $message"
}

proc cells_of { inst } {
  set block [ord::get_db_block]
  set names {}
  foreach i [[[$block findModInst $inst] getMaster] getInsts] {
    lappend names [[$i getMaster] getName]
  }
  return [lsort $names]
}

set before [cells_of bc1]

read_verilog link_hier_module1.v
expect_error "not linked yet" { replace_hier_module bc1 buffer_pair } "*STA-0201*"

link_hier_module buffer_pair
replace_hier_module bc1 buffer_pair
set after [cells_of bc1]
if { $after ne {BUF_X1 BUF_X1} } {
  error "bc1 holds $after, expected buffer_pair's two BUF_X1"
}
if { [cells_of bc2] ne $before } {
  error "bc2 changed: [cells_of bc2]"
}

expect_error "linked twice" { link_hier_module buffer_pair } "*ORD-2061*"
expect_error "a module the design has" { link_hier_module inv_chain } "*ORD-2061*"
expect_error "a module nothing read" { link_hier_module no_such_module } "*STA-1390*"
puts "pass"
