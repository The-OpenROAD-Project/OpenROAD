#package require control
#control::control assert enabled 1
#
# Converted from test_module.py
# TclTest isn't available so just calling tests sequentially
#
source "helper.tcl"

proc setUp {} {
    lassign [createSimpleDB] db lib
    set block [create1LevelBlock $db $lib [$db getChip]]
    set master_mod [odb::dbModule_create $block "master_mod"]
    set parent_mod [odb::dbModule_create $block "parent_mod"]
    set i1 [odb::dbModInst_create $parent_mod $master_mod "i1"]
    set inst1 [$block findInst "inst"]
    return [list $db $block $i1 $master_mod $parent_mod $inst1]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

proc test_default {} {
    lassign [setUp] db block i1 master_mod parent_mod inst1
    assertStringEq [$master_mod getName] "master_mod"
    assertStringEq [[$block findModule "parent_mod"] getName] "parent_mod"
    assertStringEq [$i1 getName] "i1"
    assertStringEq [[$i1 getParent] getName] "parent_mod"
    assertStringEq [[$i1 getMaster] getName] "master_mod"
    assertStringEq [[$master_mod getModInst] getName] "i1"
    assertStringEq [[lindex [$parent_mod getChildren] 0] getName] "i1"
    assertStringEq [[lindex [$block getModInsts] 0] getName] "i1"
    $parent_mod addInst $inst1
    assertStringEq [[lindex [$parent_mod getInsts] 0] getName] "inst"
    assertStringEq [[$inst1 getModule] getName] "parent_mod"
    odb::dbInst_destroy $inst1
    assertObjIsNull [$parent_mod findModInst "inst"]
    assertStringEq [[$parent_mod findModInst "i1"] getName] "i1"
    odb::dbModInst_destroy $i1
    odb::dbModule_destroy $parent_mod
    tearDown $db
}

test_default
puts "pass"
exit 0
