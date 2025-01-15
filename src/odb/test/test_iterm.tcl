#package require control
#control::control assert enabled 1
#
# Converted from test_iterm.py
# TclTest isn't available so just calling tests sequentially
#

source "helper.tcl"

proc setUp {{use_default_db 0}} {
    lassign [createSimpleDB $use_default_db] db lib
    set blockName "1LevelBlock"
    set block [odb::dbBlock_create [$db getChip] blockName]
    set and2 [$lib findMaster "and2"]
    set inst [odb::dbInst_create $block $and2 "inst"]
    set iterm_a [$inst findITerm "a"]
    return [list $db $lib $block $iterm_a $and2 $inst]
}

proc tearDown {db} {
    #odb::dbDatabase_destroy $db
}

proc test_idle {} {
    lassign [setUp] db lib block iterm_a and2 inst
    assertObjIsNull [$iterm_a getNet]
    tearDown $db
}

proc test_connection_from_iterm {} {
    lassign [setUp] db lib block iterm_a and2 inst
    # Create net and Connect
    set n [odb::dbNet_create $block "n1"]
    assert {[$n getITermCount] == 0}
    assert {[llength [$n getITerms] == 0]}
    $iterm_a connect $n
    $iterm_a setConnected 
    assertStringEq [[$iterm_a getNet] getName] "n1"
    assert {[$n getITermCount] == 1}
    assertStringEq [[[lindex [$n getITerms] 0] getMTerm] getName] "a"
    assert {[$iterm_a isConnected] == 1}
    # disconnect
    $iterm_a disconnect
    $iterm_a clearConnected
    assert {[$n getITermCount] == 0}
    assert {[llength [$n getITerms] == 0]}
    assertObjIsNull [$iterm_a getNet]
    assert {[$iterm_a isConnected] == 0}
    tearDown $db
}

proc test_connection_from_inst {} {
    lassign [setUp] db lib block iterm_a and2 inst
    # Create net and Connect
    set n [odb::dbNet_create $block "n1"]
    assert {[$n getITermCount] == 0}
    assert {[llength [$n getITerms]] == 0}
    $iterm_a connect $n
    $iterm_a setConnected 
    assertStringEq [[$iterm_a getNet] getName] "n1"
    assert {[$n getITermCount] == 1}
    assertStringEq [[[lindex [$n getITerms] 0] getMTerm] getName] "a"
    assert {[$iterm_a isConnected] == 1}
    # disconnect
    $iterm_a disconnect
    $iterm_a clearConnected
    assert {[$n getITermCount] == 0}
    assert {[llength [$n getITerms] == 0}
    assertObjIsNull [$iterm_a getNet]
    assert {[$iterm_a isConnected] == 0}
    tearDown $db
}

#
# This test calls setUp with use_default_db since we need the logger to be
# defined for this DB (the negative tests result in warnings being logged).
#
proc test_avgxy_R0 {} {
    lassign [setUp 1] db lib block iterm_a and2 inst
    # no mpin to work on - fails
    lassign [$iterm_a getAvgXY] result x y
    assert {$result == 0} "Result with no mpin was true"
    set mterm_a [$and2 findMTerm "a"]
    set mpin_a [odb::dbMPin_create $mterm_a]
    # no boxes to work on - fails
    lassign [$iterm_a getAvgXY] result x y
    assert {result == 0} "Result with no boxes was true"
    set layer [lindex [[$lib getTech] getLayers] 0]
    set geo_box_a_1 [odb::dbBox_create $mpin_a $layer 0 0 50 50]
    lassign [$iterm_a getAvgXY] result x y
    assert {result == 1} "Result with geo_box_a_1 was false"
    set expected_size [expr 50 / 2]
    assert {$x == $expected_size} [format "geo_box_a_1 x's didn't match: %d %d" $x $expected_size]
    assert {$y == $expected_size} [format "geo_box_a_1 y's didn't match: %d %d" $y $expected_size]
    set geo_box_a_2 [odb::dbBox_create $mpin_a $layer 5 10 100 100]
    lassign [$iterm_a getAvgXY] result x y
    assert {result == 1} "Result with geo_box_a_2 was false"
    set expected_x_size [expr ((0 + 50) + (5 + 100)) / 4]
    set expected_y_size [expr ((0 + 50) + (10 + 100)) / 4]
    assert {$x == $expected_x_size} [format "geo_box_a_2 x's didn't match: %d %d" $x $expected_x_size]
    assert {$y == $expected_y_size} [format "geo_box_a_2 y's didn't match: %d %d" $y $expected_y_size]
    # don't tear down DB since we are using the default DB
}

proc test_avgxy_R90 {} {
    lassign [setUp] db lib block iterm_a and2 inst
    set mterm_a [$and2 findMTerm "a"]
    set mpin_a [odb::dbMPin_create $mterm_a]
    set layer [lindex [[$lib getTech] getLayers] 0]
    set geo_box_a_1 [odb::dbBox_create $mpin_a $layer 0 0 50 50]
    set geo_box_a_2 [odb::dbBox_create $mpin_a $layer 0 0 100 100]
    $inst setOrient "R90"
    lassign [$iterm_a getAvgXY] result x y
    assert {result == 1}
    set expected_x_size [expr (((0 + 50) + (0 + 100)) / 4) * -1]
    set expected_y_size [expr ((0 + 50) + (0 + 100)) / 4]
    assert {$x == $expected_x_size}
    assert {$y == $expected_y_size}
    tearDown $db
}

test_idle
test_connection_from_iterm
test_connection_from_inst
test_avgxy_R0
test_avgxy_R90
puts "pass"
exit 0
