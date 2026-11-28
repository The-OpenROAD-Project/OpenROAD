# Prints the cells retime created (named ret_* or unmap_*): master, module,
# and the net on each pin, with the pins and ports that read each net the cell
# drives; then the instance and net counts.
proc report_new_cells { } {
  set block [ord::get_db_block]
  set lines {}
  foreach inst [$block getInsts] {
    set name [$inst getName]
    if { ![regexp {(^|/)(ret|unmap)_} $name] } {
      continue
    }
    set pins {}
    foreach iterm [$inst getITerms] {
      set net [$iterm getNet]
      if { $net == "NULL" } {
        continue
      }
      set pin "[[$iterm getMTerm] getName]=[$net getName]"
      if { [$iterm getIoType] == "OUTPUT" } {
        set readers {}
        foreach load [$net getITerms] {
          if { $load != $iterm } {
            lappend readers "[[$load getInst] getName]/[[$load getMTerm] getName]"
          }
        }
        foreach port [$net getBTerms] {
          lappend readers [$port getName]
        }
        append pin " ([join [lsort $readers]])"
      }
      lappend pins $pin
    }
    set module [[$inst getModule] getName]
    lappend lines "$name [[$inst getMaster] getName] $module: [join $pins]"
  }
  foreach line [lsort $lines] {
    puts $line
  }
  puts "instances [llength [$block getInsts]], nets [llength [$block getNets]]"
}

# Prints, for each pin (instance/terminal), the instance that drives the net
# it reads and that instance's master.
proc report_reads { pins } {
  set block [ord::get_db_block]
  foreach pin $pins {
    set driver [[[$block findITerm $pin] getNet] getFirstDriverInst]
    puts "$pin reads [$driver getName] ([[$driver getMaster] getName])"
  }
}

# Prints the number of instances that are not placed.
proc report_unplaced { } {
  set unplaced 0
  foreach inst [[ord::get_db_block] getInsts] {
    if { ![$inst isPlaced] } {
      incr unplaced
    }
  }
  puts "unplaced instances: $unplaced"
}

# Prints, for each instance name, whether the instance is still in the block.
proc report_kept { names } {
  set block [ord::get_db_block]
  foreach name $names {
    set kept [expr { [$block findInst $name] != "NULL" }]
    puts "$name: [expr { $kept ? "stays" : "removed" }]"
  }
}
