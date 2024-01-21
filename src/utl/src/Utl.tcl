global MAN_PATH 
set MAN_PATH ""

sta::define_cmd_args "man" {name}
proc man { name } {
  set DEFAULT_MAN_PATH "../docs/cat"
  global MAN_PATH
  set MAN_PATH [utl::get_input]
  if { [utl::check_valid_man_path $MAN_PATH] == false } {
    puts "Using default manpath."
    set MAN_PATH $DEFAULT_MAN_PATH
  }
  puts $DEFAULT_MAN_PATH
  puts $MAN_PATH

  set man_path $MAN_PATH
  set man_sections {}
  foreach man_section {cat1 cat2 cat3} {
      lappend man_sections "$man_path/$man_section"
  }
  set man_found false
  foreach man_section $man_sections {
    set length [string length $man_section]
    # Get suffix for man section
    set man_suffix [string range $man_section [expr {$length - 1}] $length]
    # Replace all "::" with "_"
    set name1 [string map { "::" "_" } $name]
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
              puts -nonewline "---\nPress 'q' to quit or any other key to continue: \n---"
              flush stdout;
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

namespace eval utl {

proc get_input { } {
  # Get the relative path from the user
  puts "Please enter an optional relative path to the cat folders:"
  flush stdout
  gets stdin relative_path

  # Convert the relative path to an absolute path
  set absolute_path [file join [pwd] $relative_path]

  # Display the absolute path
  puts "Absolute Path: $absolute_path"
  if { [utl::check_valid_path $absolute_path] } {
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

# utl namespace end
}