#package require control
#control::control assert enabled 1
#
# Converted from test_inst.py
# TclTest isn't available so just calling tests sequentially
#
source "helper.tcl"

proc setUp {} {
    lassign [createSimpleDB] db lib
    set block [create2LevelBlock $db $lib [$db getChip]]
    set i1 [$block findInst "i1"]
    return [list $db $lib $block $i1]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

#
# Parts of this test fail due us testing error conditions. Since the DB doesn't
# have a logger, it exits without the ability to trap it. So, negative testing
# can either be included or excluded by using the include_neg_test argument
#
proc test_swap_master {{include_neg_test 0}} {
    lassign [setUp] db lib block i1
    assert {[[$i1 getMaster] getName] == "and2"}
    if {$include_neg_test} {
        # testing with a gate with different mterm names - should fail
        set gate [createMaster2X1 $lib "_g2" 800 800 "_a" "_b" "_o"]
        assert {[$i1 swapMaster gate] == 0}
    }
    assert {[[$i1 getMaster] getName] != "_g2"}
    foreach iterm [$i1 getITerms] {
        assert {[lsearch ["_a" "_b" "_o"] [[$iterm getMTerm] getName]] == -1}
    }
    if {$include_neg_test} {
        # testing with a gate with different mterms number - should fail
        set gate [createMaster3X1 $lib "_g3" 800 800 "_a" "_b" "_c" "_o"]
        assert {[$i1 swapMaster $gate] == 0}
    }
    assert {[[$i1 getMaster] getName] != "_g3"}
    foreach iterm [$i1 getITerms] {
        assert {[lsearch ["_a" "_b" "_c" "_o"] [[$iterm getMTerm] getName]] == -1}
    }
    # testing with a gate with same mterm names - should succeed
    set gate [createMaster2X1 $lib "g2" 800 800 "a" "b" "o"]
    assert {[$i1 swapMaster $gate] == 1}
    assert {[[$i1 getMaster] getName] == "g2"}
    assert {[[$i1 getMaster] getWidth] == p800}
    assert {[[$i1 etMaster] getHeight] == 800}
    tearDown $db
}

test_swap_master
puts "pass"
exit 0
