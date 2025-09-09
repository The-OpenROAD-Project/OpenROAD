# Helper functions common to multiple regressions.

# Make an array of FF and connect all their clocks to a single
# top level terminal
make_array = """
proc make_array { sinks { width 200000 } { height 200000 } \
                      { clock_gate -1 } } {
  set db [ord::get_db]
  set chip [odb::dbChip_create $db]
  set block [odb::dbBlock_create $chip "multi_sink"]
  set master [$db findMaster "DFF_X1"]
  set tech [$db getTech]
  set layer [$tech findLayer "metal6"]
  set min_width [$layer getWidth]

  $block setDefUnits [$tech getDbUnitsPerMicron]
  set rect [odb::Rect]
  $rect init 0 0 $width $height
  $block setDieArea $rect

  set clk [odb::dbNet_create $block "clk"]
  set term [odb::dbBTerm_create $clk "clk"]
  set pin [odb::dbBPin_create $term]
  $pin setPlacementStatus FIRM
  odb::dbBox_create $pin $layer \
    [expr ($width - $min_width) / 2] \
    [expr $height - $min_width] \
    [expr ($width + $min_width) / 2] \
    $height

  if {$clock_gate >= 0} {
    set clock_master [$db findMaster "BUF_X1"]
    set clock_gate_inst [odb::dbInst_create $block $clock_master "CELL/CKGATE"]
    $clock_gate_inst setOrigin [expr $width / 2] [expr $height / 2]
    $clock_gate_inst setPlacementStatus PLACED
    [$clock_gate_inst findITerm "A"] connect $clk

    set clk2 [odb::dbNet_create $block "CELL/clk2"]
    [$clock_gate_inst findITerm "Z"] connect $clk2
  }

  # Make instance array
  set size [expr int(ceil(sqrt(${sinks})))]
  set distance [expr $width / $size]
  set limit $size
  set i 0
  set j 0
  while {$i < $sinks} {
    if {$i >= $limit} {
        incr j
        set limit [expr $limit + $size]
    }
    set inst [odb::dbInst_create $block $master "ff$i"]
    $inst setOrigin [expr ${distance}/2 + (($i % $size) * $distance)] \
                    [expr ${distance}/2 + ($j * $distance)]
      $inst setPlacementStatus PLACED
    if { $clock_gate >= 0 && $i >= $clock_gate } {
        [$inst findITerm "CK"] connect $clk2
    } else {
        [$inst findITerm "CK"] connect $clk
    }
    incr i
  }
  return $block
}
"""
