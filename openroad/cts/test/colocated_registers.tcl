# CTS connects both registers of a pair placed at the same location.
#
# ffa_twin sits exactly on ffa_112 (clock clk_a, 226 sinks, so its sinks
# are clustered) and ffb_twin exactly on ffb_14 (clock clk_b, 37 sinks, too
# few to cluster, so the H-tree's leaf level connects them). HTreeBuilder
# found a sink by its location, one sink per location, so one register of
# each pair was left out of the tree with its clock pin on no net, and
# nothing said so.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def colocated_registers.def

create_clock -name clk_a -period 5 [get_ports clk_a]
create_clock -name clk_b -period 5 [get_ports clk_b]
source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

set block [ord::get_db_block]

proc location { name } {
  return [[$::block findInst $name] getLocation]
}

check "ffa_twin is on top of ffa_112" \
  { expr { [location ffa_twin] == [location ffa_112] } } 1
check "ffb_twin is on top of ffb_14" \
  { expr { [location ffb_twin] == [location ffb_14] } } 1

tee -quiet -variable log {
  clock_tree_synthesis -root_buf CLKBUF_X3 \
    -buf_list CLKBUF_X3 \
    -sink_clustering_enable \
    -sink_clustering_size 10
}
check "only clk_a's sinks are clustered" { regexp -all {CTS-0019} $log } 1

# The port a register's clock comes from, through the clock tree's buffers.
proc clock_source { name } {
  set net [[[$::block findInst $name] findITerm CK] getNet]
  for { set depth 0 } { $depth < 32 } { incr depth } {
    if { $net eq "NULL" } {
      return "no net"
    }
    foreach bterm [$net getBTerms] {
      return [$bterm getName]
    }
    set input ""
    foreach iterm [$net getITerms] {
      if { [$iterm isOutputSignal] } {
        foreach driver_iterm [[$iterm getInst] getITerms] {
          if {
            [$driver_iterm isInputSignal]
            && [$driver_iterm getNet] ne "NULL"
          } {
            set input $driver_iterm
          }
        }
      }
    }
    if { $input eq "" } {
      return "no driver"
    }
    set net [$input getNet]
  }
  return "no port"
}

set pairs { ffa_112 clk_a ffa_twin clk_a ffb_14 clk_b ffb_twin clk_b }
foreach { name port } $pairs {
  check "$name is clocked from $port" { clock_source $name } $port
}

set unclocked 0
foreach inst [$block getInsts] {
  if {
    [[$inst getMaster] getName] eq "DFF_X1"
    && [clock_source [$inst getName]] ni { clk_a clk_b }
  } {
    incr unclocked
  }
}
check "every register is clocked from a port" { set unclocked } 0

exit_summary
