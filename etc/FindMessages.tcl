#!/bin/sh
# The next line is executed by /bin/sh, but not Tcl \
exec tclsh $0 ${1+"$@"}

############################################################################
##
## Copyright (c) 2019, OpenROAD
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
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
############################################################################

# Find logger warn/error/critical calls and report sorted message IDs.
# Also checks for duplicate message IDs.
#
# Useage:
# cd src/<tool>
# ../etc/FindMessages.tcl > messages.txt

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
  set files_c [concat $files_c [glob -nocomplain [file join $subdir "*" "*.{cc,,cpp,h,hh,yy,ll,i}"]]]
}
set warn_regexp_c {->(info|warn|fileWarn|error|fileError|critical)\((?:utl::)?([A-Z][A-Z][A-Z]), *([0-9]+),.*(".+")}

set files_tcl {}
foreach subdir $subdirs {
  set files_tcl [concat $files_tcl [glob -nocomplain [file join $subdir "*.tcl"]]]
}
set warn_regexp_tcl {(info|warn|error) ("[A-Z][A-Z][A-Z]"|[A-Z][A-Z][A-Z]) ([0-9]+) (".+")}

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
      puts "Warning: $msg_id duplicated"
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

################################################################

set msgs {}
scan_files $files_c $warn_regexp_c
scan_files $files_tcl $warn_regexp_tcl
check_msgs
report_msgs
