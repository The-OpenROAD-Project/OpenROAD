proc write_nets { filename } {
  foreach net [get_nets *] {
    set pins [get_pins -of_object $net]
    set pin_count [llength $pins]
    if { $pin_count > 2 } {
      puts "Net [get_full_name $net] $pin_count"
      set drvr "NULL"
      foreach pin $pins {
        if { [$pin is_driver] } {
          set drvr $pin
          break
        }
      }
      if { $drvr != "NULL" } {
        puts $stream [get_full_name $drvr]
        foreach pin $pins {
          if { $pin != $drvr } {
            puts $stream [get_full_name $pin]
          }
        }
      }
    }
  }
}
