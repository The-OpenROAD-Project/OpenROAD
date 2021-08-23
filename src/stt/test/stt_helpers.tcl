# write nets for unit test
proc write_nets { filename } {
  set stream [open $filename "w"]
  foreach net [get_nets *] {
    write_net $net $stream
  }
  close $stream
}

proc write_net { net stream } {
  set pins [get_pins -of_object $net]
  set ports [get_ports -of_object $net]
  set pin_count [expr [llength $ports] + [llength $pins]]
  if { $pin_count > 2 } {
    set drvr "NULL"
    set drvr_index 0
    foreach port $ports {
      if { [sta::port_direction $port] == "input" } {
        set drvr $port
        break
      }
      incr drvr_index
    }
    if { $drvr == "NULL" } {
      foreach pin $pins {
        if { [$pin is_driver] } {
          set drvr $pin
          break
        }
        incr drvr_index
      }
    }
    if { $drvr != "NULL" } {
      puts $stream "Net [get_full_name $net] $drvr_index"
      foreach port $ports {
        write_pin $port $stream
      }
      foreach pin $pins {
        write_pin $pin $stream
      }
    }
    puts $stream ""
  }
}

proc write_pin { pin stream } {
  if { [sta::object_type $pin] == "Port" } {
    lassign [sta::port_location $pin] x y
  } else {
    lassign [sta::pin_location $pin] x y
  }
  puts $stream "[get_full_name $pin] [sta::format_distance $x 2 ] [sta::format_distance $y 2]"
}

# Each net is
# {net_name pin_count drvr_index {pin_name x y}...}
proc read_nets { filename } {
  set stream [open $filename "r"]
  set nets {}
  while { 1 } {
    gets $stream line
    if { [eof $stream] } {
      break
    }
    lassign $line ignore net_name drvr_index
    set pins {}
    while { 1 } {
      gets $stream line
      if { $line == "" } {
        break
      }
      lassign $line pin_name x y
      lappend pins [list $pin_name $x $y]
    }
    set net [concat [list $net_name $drvr_index] $pins]
    lappend nets $net
  }
  close $stream
  return $nets
}

proc write_gcd_nets {} {
  read_liberty Nangate45/Nangate45_typ.lib
  read_lef Nangate45/Nangate45.lef
  read_def ../../rsz/test/gcd_nangate45_placed.def
  write_nets "gcd.nets"
}

proc report_stt_net { net alpha } {
  set pins [lassign $net net_name drvr_index]
  puts "Net $net_name"
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  stt::report_stt_tree $xs $ys $drvr_index $alpha
}

proc report_pd_net { net alpha } {
  set pins [lassign $net net_name drvr_index]
  puts "Net $net_name"
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  stt::report_pd_tree $xs $ys $drvr_index $alpha
}

proc report_pdrev_net { net alpha } {
  set pins [lassign $net net_name drvr_index]
  puts "Net $net_name"
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  stt::report_pdrev_tree $xs $ys $drvr_index $alpha
}

proc report_flute_net { net } {
  set pins [lassign $net net_name drvr_index]
  puts "Net $net_name"
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  stt::report_flute_tree $xs $ys $drvr_index
}

proc find_net { nets net_name } {
  foreach net $nets {
  set pins [lassign $net name drvr_index]
    if { $name == $net_name } {
      return $net
    }
  }
  return {}
}

proc highlight_pd_net { net alpha } {
  set pins [lassign $net net_name drvr_index]
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  stt::highlight_pd_tree $xs $ys $drvr_index $alpha
}

proc highlight_stt_net { net alpha } {
  set pins [lassign $net net_name drvr_index]
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  stt::highlight_stt_tree $xs $ys $drvr_index $alpha
}

proc highlight_flute_net { net } {
  set pins [lassign $net net_name drvr_index]
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  stt::highlight_flute_tree $xs $ys
}
