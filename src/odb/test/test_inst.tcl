#package require control
#control::control assert enabled 1
#
# Converted from test_inst.py
# TclTest isn't available so just calling tests sequentially
#
source "helper.tcl"

proc setUp {{use_default_db 0}} {
    lassign [createSimpleDB $use_default_db] db lib
    set block [create2LevelBlock $db $lib [$db getChip]]
    set i1 [$block findInst "i1"]
    return [list $db $lib $block $i1]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

#
# This test calls setUp with use_default_db since we need the logger to be
# defined for this DB (the negative tests result in warnings being logged).
#
proc test_swap_master {} {
    lassign [setUp 1] db lib block i1
    assertStringEq [[$i1 getMaster] getName] "and2"
    # testing with a gate with different mterm names - should fail
    set gate [createMaster2X1 $lib "_g2" 800 800 "_a" "_b" "_o"]
    assert {[$i1 swapMaster gate] == 0}
    assertStringNotEq [[$i1 getMaster] getName] "_g2"
    foreach iterm [$i1 getITerms] {
        assert {[lsearch ["_a" "_b" "_o"] [[$iterm getMTerm] getName]] == -1}
    }
    # testing with a gate with different mterms number - should fail
    set gate [createMaster3X1 $lib "_g3" 800 800 "_a" "_b" "_c" "_o"]
    assert {[$i1 swapMaster $gate] == 0}
    assertStringNotEq [[$i1 getMaster] getName] "_g3"
    foreach iterm [$i1 getITerms] {
        assert {[lsearch ["_a" "_b" "_c" "_o"] [[$iterm getMTerm] getName]] == -1}
    }
    # testing with a gate with same mterm names - should succeed
    set gate [createMaster2X1 $lib "g2" 800 800 "a" "b" "o"]
    assert {[$i1 swapMaster $gate] == 1}
    assertStringEq [[$i1 getMaster] getName] "g2"
    assert {[[$i1 getMaster] getWidth] == p800}
    assert {[[$i1 etMaster] getHeight] == 800}
    # don't tear down DB since we are using the default DB
}

test_swap_master
puts "pass"
exit 0
