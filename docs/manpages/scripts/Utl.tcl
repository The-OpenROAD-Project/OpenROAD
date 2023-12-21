global MAN_PATH 
set MAN_PATH ""

proc get_input { } {
  # Get the relative path from the user
  puts "Please enter the relative path to the cat folders:"
  flush stdout
  gets stdin relative_path

  # Convert the relative path to an absolute path
  set absolute_path [file join [pwd] $relative_path]

  # Display the absolute path
  puts "Absolute Path: $absolute_path"
  if { [check_valid_path $absolute_path] } {
    return $absolute_path
  }
  return ""
}

proc check_valid_path { path } {
  if {[file isdirectory $path]} {
    return true 
  } else {
    puts "Invalid path, please retry."
    return false
  }
}

proc check_valid_man_path { path } {
  if {[file isdirectory "$path/man1"]} {
    return true
  } else {
    puts "Invalid man path, please retry."
    return false
  }
}

proc man { name } {
  global MAN_PATH
  if { [string length $MAN_PATH] == 0 } {
    set MAN_PATH [get_input]
    if { $MAN_PATH == 0 } { return }
  }

  if { [check_valid_man_path $MAN_PATH] == 0 } {
    set $MAN_PATH ""
    puts ""
  }

  set man_path $MAN_PATH
  set man_sections {}
  foreach man_section {man1 man2 man3} {
      lappend man_sections "$man_path/$man_section"
  }
  set man_found false
  foreach man_section $man_sections {
    set length [string length $man_section]
    # Get suffix for man section
    set man_suffix [string range $man_section [expr {$length - 1}] $length]
    set name1 $name
    append name1 ".$man_suffix"
    set man_file [file join $man_section $name1]
    if { [file exists $man_file] } {
        set man_found true
        set file_handle [open $man_file r]
        set content [read $file_handle]
        close $file_handle

        # Display content
        set lines [split $content "\n"]
        set num_lines [llength $lines]
        set page_size 10

        for {set i 0} {$i < $num_lines} {incr i $page_size} {
          set page [lrange $lines $i [expr {$i + $page_size - 1}]]
          puts [join $page "\n"]

          # Ask user to continue or quit
          if {[llength $lines] > $page_size} {
              puts -nonewline "Press 'q' to quit or any other key to continue: "
              flush stdout
              set input [gets stdin]
              if {$input == "q"} {
                break
              }
            }
          }
        }
      }

  if { $man_found == false } {
    utl::error UTL 100 "Man page for $name is not found."
  }
}

