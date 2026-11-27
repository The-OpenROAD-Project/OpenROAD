# Checks on the clock tree CTS builds, for tests that assert what the tree
# must satisfy rather than the log that built it.

# The block terminal a clock pin is driven from, through the clock tree's
# buffers and inverters; "no net" or "no driver" when the tree is broken.
proc clock_tree_source { iterm } {
  set net [$iterm getNet]
  for { set depth 0 } { $depth < 64 } { incr depth } {
    if { $net eq "NULL" } {
      return "no net"
    }
    foreach bterm [$net getBTerms] {
      return [$bterm getName]
    }
    set driver ""
    foreach net_iterm [$net getITerms] {
      if { [$net_iterm isOutputSignal] } {
        set driver $net_iterm
      }
    }
    if { $driver eq "" } {
      return "no driver"
    }
    set input ""
    foreach inst_iterm [[$driver getInst] getITerms] {
      if { [$inst_iterm isInputSignal] && [$inst_iterm getNet] ne "NULL" } {
        set input $inst_iterm
      }
    }
    if { $input eq "" } {
      return "no input"
    }
    set net [$input getNet]
  }
  return "too deep"
}

# The pin_name iterms of the instances whose names match inst_pattern.
proc clock_pins { block inst_pattern pin_name } {
  set pins {}
  foreach inst [$block getInsts] {
    if { [string match $inst_pattern [$inst getName]] } {
      lappend pins [$inst findITerm $pin_name]
    }
  }
  return $pins
}

# How many of iterms are not driven from port through the clock tree.
proc unclocked_count { iterms port } {
  set unclocked 0
  foreach iterm $iterms {
    if { [clock_tree_source $iterm] ne $port } {
      incr unclocked
    }
  }
  return $unclocked
}
