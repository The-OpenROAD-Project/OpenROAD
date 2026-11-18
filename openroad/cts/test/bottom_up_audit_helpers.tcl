# Independent connectivity checks, deliberately not using CTS's audit walker.
proc check_bottom_up_paths { block originals log } {
  set pending [list [$block findNet clk]]
  set visited [dict create]
  set pins [dict create]
  while { [llength $pending] > 0 } {
    set net [lindex $pending end]
    set pending [lrange $pending 0 end-1]
    set name [$net getName]
    if { [dict exists $visited $name] } {
      continue
    }
    dict set visited $name 1
    foreach pin [$net getITerms] {
      if { ![$pin isInputSignal] } {
        continue
      }
      dict set pins [$pin getName] 1
      foreach output [[$pin getInst] getITerms] {
        if { ![$output isOutputSignal] } {
          continue
        }
        set next [$output getNet]
        if { $next != "NULL" && [$next getSigType] == "CLOCK" } {
          lappend pending $next
        }
      }
    }
  }
  foreach pin $originals {
    if { [$pin isInputSignal] } {
      check "original sink [$pin getName] has a root path" {
        dict exists $pins [$pin getName]
      } 1
    }
  }
  foreach net [$block getNets] {
    if { [string match "clknet_*" [$net getName]] } {
      check "generated subnet [$net getName] has a root path" {
        dict exists $visited [$net getName]
      } 1
    }
  }
  check "audit reports complete coverage" {
    regexp {audit coverage: ([0-9]+)/([0-9]+) original sinks, ([0-9]+)/([0-9]+) generated subnets} \
      $log matched reached_sinks expected_sinks reached_nets expected_nets
  } 1
  check "all modeled original sinks are audited" { expr { $reached_sinks == $expected_sinks } } 1
  check "all modeled subnets are audited" { expr { $reached_nets == $expected_nets } } 1
  check "audit visits beyond the source net" { expr { [dict size $visited] > 1 } } 1
}
