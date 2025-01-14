#package require control
#control::control assert enabled 1
#
# Converted from test_net.py
# TclTest isn't available so just calling tests sequentially
#
source "helper.tcl"

proc changeAndTest { obj SetterName GetterName expectedVal args } {
    $obj $SetterName $args
    if {[string is ascii $expectedVal]} {
        assertStringEq [$obj $GetterName] $expectedVal [format "%s/%s failed: %s %s" $SetterName $GetterName [$obj $GetterName] $expectedVal]
    } else {
        assert {[$obj $GetterName] == $expectedVal} [format "%s/%s failed: %g %g" $SetterName $GetterName [$obj $GetterName] $expectedVal]
    }
}

# Set up data
proc setUp {} {
    lassign [createSimpleDB] db lib
    set block [create1LevelBlock $db $lib [$db getChip]]
    set inst [lindex [$block getInsts] 0]
    set n1 [[$inst findITerm "a"] getNet]
    set n2 [[$inst findITerm "b"] getNet]
    set n3 [[$inst findITerm "o"] getNet]
    return [list $db $n1 $n2 $n3]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

proc test_naming {} {
    lassign [setUp] db n1 n2 n3
    changeAndTest $n1 "rename" "getName" "_n1" "_n1"
    assertStringEq [$n1 getConstName] "_n1" [format "getConstName failed: %s _n1" [$n1 getConstName]]
    assert {[$n1 rename "n2"] == 0} [format "rename failed: %s" [$n1 getName]]
    tearDown $db
}

proc test_dbSetterAndGetter {} {
    lassign [setUp] db n1 n2 n3
    changeAndTest $n1 "setRCDisconnected" "isRCDisconnected" 0 0
    changeAndTest $n1 "setRCDisconnected" "isRCDisconnected" 1 1
    changeAndTest $n1 "setWeight" "getWeight" 2 2
    changeAndTest $n1 "setSourceType" "getSourceType" "NETLIST" "NETLIST"
    changeAndTest $n1 "setXTalkClass" "getXTalkClass" 1 1
    changeAndTest $n1 "setCcAdjustFactor" "getCcAdjustFactor" 1.0 1.0
    changeAndTest $n1 "setSigType" "getSigType" "RESET" "RESET"
    tearDown $db
}

proc test_dbCc {} {
    lassign [setUp] db n1 n2 n3
    changeAndTest $n1 "setDbCc" "getDbCc" 2.0 2.0
    changeAndTest $n1 "addDbCc" "getDbCc" 5.0 3.0
    tearDown $db
}

proc test_cc {} {
    lassign [setUp] db n1 n2 n3
    set node2 [odb::dbCapNode_create $n2 0 false]
    assert {[string length $node2] != 0} [format "node2 from %s is null" [$n2 getName]]
    set node1 [odb::dbCapNode_create $n1 1 false]
    assert {[string length $node1] != 0} [format "node1 from %s is null" [$n1 getName]]
    $node1 setInternalFlag
    set ccseg [odb::dbCCSeg_create $node1 $node2]
    $n1 calibrateCouplingCap
    assert {[$n1 maxInternalCapNum] == 1}
    assert {[$n1 groundCC 1] == 1}
    assert {[$n2 groundCC 1] == 0}
    assert {[$n1 getCcCount] == 1}
    tearDown $db
}

test_naming
test_dbSetterAndGetter
test_dbCc
test_cc

puts "pass"
exit 0
