namespace eval dummy_cdl {
  proc dummy_write_cell {ch cell} {
    puts -nonewline $ch ".SUBCKT [$cell getName]"
    foreach mTerm [lreverse [$cell getMTerms]] {
      puts -nonewline $ch " [$mTerm getName]"
    }
    puts $ch ""
    puts $ch ".ENDS"
  }

  proc dummy_cdl_masters {file_name} {
    if {[catch {set ch [open $file_name "w"]} msg]} {
      error $msg
    }
    set db [ord::get_db]
    foreach lib [$db getLibs] {
      foreach cell [$lib getMasters] {
        if {[$cell isFiller]} {continue}
        dummy_write_cell $ch $cell
      }
    }
    close $ch
  }

  namespace export dummy_cdl_masters
  namespace ensemble create
}


