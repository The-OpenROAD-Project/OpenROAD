#!/bin/sh
# The next line is executed by /bin/sh, but not Tcl \
exec tclsh $0 ${1+"$@"}

# Parallax Static Timing Analyzer
# Copyright (c) 2020, Parallax Software, Inc.
# All rights reserved.
# 
# No part of this document may be copied, transmitted or
# disclosed in any form or fashion without the express
# written consent of Parallax Software, Inc.

proc scan_file { file warn_regexp } {
  global msgs

  if { [file exists $file] } {
    set in_stream [open $file r]
    gets $in_stream line
    set file_line 1

    while { ![eof $in_stream] } {
      if { [regexp -- $warn_regexp $line ignore1 ignore2 tool_id msg_id msg] } {
        lappend msgs "$tool_id $msg_id $file $file_line $msg"
      }
      gets $in_stream line
      incr file_line
    }
    close $in_stream
  } else {
    puts "Warning: file $file not found."
  }
}


set subdirs {src}
set files_c {}
foreach subdir $subdirs {
  set files_c [concat $files_c [glob -nocomplain [file join $subdir "*.{cc,,cpp,h,hh,yy,ll,i}"]]]
}
set warn_regexp_c {->(warn|fileWarn|error|fileError|critical)\(([A-Z][A-Z][A-Z]), *([0-9]+),.*(".+")}

set files_tcl {}
foreach subdir $subdirs {
  set files_tcl [concat $files_tcl [glob -nocomplain [file join $subdir "*.tcl"]]]
}
set warn_regexp_tcl {(warn|error) ([A-Z][A-Z][A-Z]) ([0-9]+) (".+")}

proc scan_files {files warn_regexp } {
  foreach file $files {
    scan_file $file $warn_regexp
  }
}

proc check_msgs { } {
  global msgs

  set msgs [lsort -index 1 -integer $msgs]
  set prev_id -1
  foreach msg $msgs {
    set msg_id [lindex $msg 1]
    if { $msg_id == $prev_id } {
      puts "Warning: $id duplicated"
    }
    set prev_id $msg_id
  }
}

proc report_msgs { } {
  global msgs

  foreach msg $msgs {
    lassign $msg tool_id msg_id file line msg1
    puts "$tool_id [format %04d $msg_id] [format %-25s [file tail $file]:$line] $msg1"
  }
}

set msgs {}
scan_files $files_c $warn_regexp_c
scan_files $files_tcl $warn_regexp_tcl
check_msgs
report_msgs
