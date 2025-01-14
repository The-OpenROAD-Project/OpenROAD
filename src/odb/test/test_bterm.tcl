#package require control
#control::control assert enabled 1
#
# Converted from test_bterm.py
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
    set n_a [odb::dbNet_create $block "na"]
    set n_b [odb::dbNet_create $block "nb"]
    set bterm_a [odb::dbBTerm_create $n_a "IN_a"]
    return [list $db $bterm_a $n_a $n_b]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

proc test_idle {} {
    lassign [setUp] db bterm_a n_a n_b
    assertStringEq [[$bterm_a getNet] getName] "na"
    assert {[$n_a getBTermCount] == 1}
    assertStringEq [[lindex [$n_a getBTerms] 0] getName] "IN_a"
    assert {[$n_b getBTermCount] == 0}
    tearDown $db
}

proc test_connect {} {
    lassign [setUp] db bterm_a n_a n_b
    $bterm_a connect $n_b
    assertStringEq [[$bterm_a getNet] getName] "nb"
    assert {[$n_a getBTermCount] == 0}
    assert {[llength [$n_a getBTerms] == 0}
    assert {[$n_b getBTermCount] == 1}
    assertStringEq [[lindex [$n_b getBTerms] 0] getName] "IN_a"
    tearDown $db
}

proc test_disconnect {} {
    lassign [setUp] db bterm_a n_a n_b
    $bterm_a disconnect
    assertObjIsNull [$bterm_a getNet]
    assert {[$n_a getBTermCount()] == 0}
    assert {[llength [$n_a getBTerms]] == 0}
    tearDown $db
}

test_idle
test_connect
test_disconnect
puts "pass"
exit 0
