#package require control
#control::control assert enabled 1
#
# Converted from test_iterm.py
# TclTest isn't available so just calling tests sequentially
#

source "helper.tcl"

proc setUp {} {
    lassign [createSimpleDB] db lib
    set blockName "1LevelBlock"
    set block [odb::dbBlock_create [$db getChip] blockName]
    set and2 [$lib findMaster "and2"]
    set inst [odb::dbInst_create $block $and2 "inst"]
    set iterm_a [$inst findITerm "a"]
    return [list $db $lib $block $iterm_a $and2 $inst]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

proc test_idle {} {
    lassign [setUp] db lib block iterm_a and2 inst
    assert {[$iterm_a getNet] == "NULL"}
    tearDown $db
}

proc test_connection_from_iterm {} {
    lassign [setUp] db lib block iterm_a and2 inst
    # Create net and Connect
    set n [odb::dbNet_create $block "n1"]
    assert {[$n getITermCount] == 0}
    assert {[lequal [$n getITerms] []]}
    $iterm_a connect $n
    $iterm_a setConnected 
    assert {[[$iterm_a getNet] getName] == "n1"}
    assert {[$n getITermCount] == 1}
    assert {[[[lindex [$n getITerms] 0] getMTerm] getName] == "a"}
    assert {[$iterm_a isConnected] == 1}
    # disconnect
    $iterm_a disconnect
    $iterm_a clearConnected
    assert {[$n getITermCount] == 0}
    assert {[lequal [$n getITerms] []]}
    assert {[$iterm_a getNet] == "NULL"}
    assert {[$iterm_a isConnected] == 0}
    tearDown $db
}

proc test_connection_from_inst {} {
    lassign [setUp] db lib block iterm_a and2 inst
    # Create net and Connect
    set n [odb::dbNet_create $block "n1"]
    assert {[$n getITermCount] == 0}
    assert {[lequal [$n getITerms] []]}
    $iterm_a connect $n
    $iterm_a setConnected 
    assert {[[$iterm_a getNet] getName] == "n1"}
    assert {[$n getITermCount] == 1}
    assert {[[[lindex [$n getITerms] 0] getMTerm] getName] == "a"}
    assert {[$iterm_a isConnected] == 1}
    # disconnect
    $iterm_a disconnect
    $iterm_a clearConnected
    assert {[$n getITermCount] == 0}
    assert {[lequal [$n getITerms] []]}
    assert {[$iterm_a getNet] == "NULL"}
    assert {[$iterm_a isConnected] == 0}
    tearDown $db
}

#
# Parts of this test fail due us testing error conditions. Since the DB doesn't
# have a logger, it exits without the ability to trap it. So, negative testing
# can either be included or excluded by using the include_neg_test argument
#
proc test_avgxy_R0 {{include_neg_test 0}} {
    lassign [setUp] db lib block iterm_a and2 inst
    if {$include_neg_test} {
        lassign [$iterm_a getAvgXY] result x y
        assert {$result == 0}  # no mpin to work on - fails
    }
    set mterm_a [$and2 findMTerm "a"]
    set mpin_a [odb::dbMPin_create $mterm_a]
    if {$include_neg_test} {
        lassign [$iterm_a getAvgXY] result x y
        assert {result == 0}  # no boxes to work on
    }
    set layer [lindex [[$lib getTech] getLayers] 0]
    set geo_box_a_1 [odb::dbBox_create $mpin_a $layer 0 0 50 50]
    lassign [$iterm_a getAvgXY] result x y
    assert {result == 1}
    set expected_size [expr 50 / 2]
    assert {$x == $expected_size}
    assert {$y == $expected_size}
    set geo_box_a_2 [odb::dbBox_create $mpin_a $layer 5 10 100 100]
    lassign [$iterm_a getAvgXY] result x y
    assert {result == 1}
    set expected_x_size [expr ((0 + 50) + (5 + 100)) / 4]
    set expected_y_size [expr ((0 + 50) + (10 + 100)) / 4]
    assert {$x == $expected_x_size}
    assert {$y == $expected_y_size}
    tearDown $db
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
