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

proc report_pdrev_net { net alpha use_pd } {
  set pins [lassign $net net_name drvr_index]
  puts "Net $net_name"
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    lappend xs $x
    lappend ys $y
  }
  if { $use_pd } {
    pdr::report_pd_tree $xs $ys $drvr_index $alpha
  } else {
    pdr::report_pdII_tree $xs $ys $drvr_index $alpha
  }
}

proc report_pdrev_nets { nets alpha use_pd } {
  foreach net $nets {
    report_pdrev_net $net $alpha $use_pd
  }
}

proc find_pdrev_net { nets net_name } {
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
  puts "Net $net_name"
  set xs {}
  set ys {}
  foreach pin $pins {
    lassign $pin pin_name x y
    #set x [ord::microns_to_dbu $x]
    #set y [ord::microns_to_dbu $y]
    lappend xs $x
    lappend ys $y
  }
  pdr::highlight_pd_tree $xs $ys $drvr_index $alpha
}
