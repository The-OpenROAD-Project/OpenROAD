###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2024, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

# This code contains the Tcl-native implementation of man command.

global MAN_PATH
set MAN_PATH ""

sta::define_cmd_args "man" { name\
                            [-manpath manpath]\
                            [-no_pager]}
proc man { args } {
  sta::parse_key_args "man" args \
    keys {-manpath} flags {-no_pager}

  set name [lindex $args 0]

  set DEFAULT_MAN_PATH [file join [ord::get_docs_path] "cat"]

  global MAN_PATH
  if { [info exists keys(-manpath)] } {
    set MAN_PATH $keys(-manpath)
    if { [utl::check_valid_man_path $MAN_PATH] == false } {
      puts "Using default manpath."
      set MAN_PATH $DEFAULT_MAN_PATH
    }
  } else {
    set MAN_PATH $DEFAULT_MAN_PATH
  }

  set no_pager 0
  if { [info exists flags(-no_pager)] } {
    set no_pager 1
  }

  if { [gui::enabled] && !$no_pager } {
    set no_pager 1
  }

  set man_path $MAN_PATH
  set man_sections {}
  foreach man_section {cat1 cat2 cat3} {
    lappend man_sections "$man_path/$man_section"
  }
  set man_found false
  foreach man_section $man_sections {
    set length [string length $man_section]
    # Get suffix for man section
    set man_suffix [string range $man_section [expr { $length - 1 }] $length]
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
      set page_size 40

      for { set i 0 } { $i < $num_lines } { incr i $page_size } {
        set page_end [expr { $i + $page_size - 1 }]
        set page [lrange $lines $i $page_end]
        puts [join $page "\n"]

        # Ask user to continue or quit
        if { !$no_pager && $num_lines > $page_size && $page_end < $num_lines } {
          puts -nonewline "---\nPress 'q' to quit or any other key to continue: \n---"
          flush stdout
          set input [gets stdin]
          if { $input == "q" } {
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

sta::define_cmd_args "tee" {-file filename
                            -variable name
                            [-append]
                            [-quiet]
                            command}
proc tee { args } {
  sta::parse_key_args "tee" args \
    keys {-file -variable} flags {-append -quiet}

  sta::check_argc_eq1 "tee" $args

  if { ![info exists keys(-file)] && ![info exists keys(-variable)] } {
    utl::error UTL 101 "-file or -variable is required"
  }

  if { [info exists flags(-quiet)] } {
    if { [info exists keys(-variable)] } {
      utl::redirectStringBegin
    } else {
      if { [info exists flags(-append)] } {
        utl::redirectFileAppendBegin $keys(-file)
      } else {
        utl::redirectFileBegin $keys(-file)
      }
    }
  } else {
    if { [info exists keys(-variable)] } {
      utl::teeStringBegin
    } else {
      if { [info exists flags(-append)] } {
        utl::teeFileAppendBegin $keys(-file)
      } else {
        utl::teeFileBegin $keys(-file)
      }
    }
  }

  global errorCode errorInfo
  set code [catch { eval { {*}[lindex $args 0] } } ret]

  if { [info exists keys(-variable)] } {
    if { [info exists flags(-quiet)] } {
      set stream [utl::redirectStringEnd]
    } else {
      set stream [utl::teeStringEnd]
    }
    upvar 1 $keys(-variable) var
    if { [info exists flags(-append)] } {
      if { ![info exists var] } {
        set var ""
      }
      set var "$var$stream"
    } else {
      set var $stream
    }
  } else {
    if { [info exists flags(-quiet)] } {
      utl::redirectFileEnd
    } else {
      utl::teeFileEnd
    }
  }

  if { $code == 1 } {
    return -code $code -errorcode $errorCode -errorinfo $errorInfo $ret
  } else {
    return $ret
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
  if { [file isdirectory $path] } {
    return true
  } else {
    puts "Invalid path, please retry."
    return false
  }
}

proc check_valid_man_path { path } {
  if { [file isdirectory "$path/cat1"] } {
    return true
  } else {
    puts "Invalid man path, please retry."
    return false
  }
}

# utl namespace end
}
