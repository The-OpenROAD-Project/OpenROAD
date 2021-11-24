source "helpers.tcl"
set header {VERSION 5.8 ; 
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;

DESIGN multi_sink ;

UNITS DISTANCE MICRONS 2000 ;

DIEAREA ( 0 0 ) ( 200000 200000 ) ;
}

set middle {
PINS 1 ;
- clk + NET clk + DIRECTION INPUT + USE SIGNAL 
  + LAYER metal6 ( -140 0 ) ( 140 280 ) + FIXED ( 100000 200000 ) S ;
END PINS
}

proc write_high_sinks_def { filename sinks } {
  global header middle

  set stream [open $filename "w"]
  puts $stream $header
  puts $stream "COMPONENTS $sinks ;"
  set space 200000
  set size [expr int(ceil(sqrt(${sinks})))]
  set distance [expr $space / $size]
  set limit $size
  set i 0
  set j 0
  while {$i < $sinks} {
    if {$i >= $limit} {
        incr j
        set limit [expr $limit + $size]
    }
    puts $stream "- ff$i DFF_X1 + PLACED   ( [expr ${distance}/2 + (($i % $size) * $distance)] [expr ${distance}/2 + ($j * $distance)] ) N ;"
    incr i
  }
  puts $stream "END COMPONENTS"

  puts $stream $middle

  puts $stream "NETS 2 ;"
  puts $stream "- clk ( PIN clk )"
  set i 0
  while {$i < $sinks} {
    puts -nonewline $stream " ( ff$i CK )"
    if { [expr $i % 10] == 0 } {
      puts $stream ""
    }
    incr i
  }
  puts $stream " ;"

  puts $stream "END NETS"

  puts $stream "END DESIGN"
  close $stream
}
set def_filename [file join $result_dir "clustered_test.def"]

write_high_sinks_def $def_filename 300

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename

create_clock -period 5 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

clock_tree_synthesis -root_buf CLKBUF_X3 \
                     -buf_list CLKBUF_X3 \
                     -wire_unit 20 \
                     -sink_clustering_enable \
                     -distance_between_buffers 100 \
                     -num_static_layers 1

exit
