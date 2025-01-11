#package require control
#control::control assert enabled 1
#
# Converted from test_group.py
# TclTest isn't available so just calling tests sequentially
#
source "helper.tcl"

proc setUp {} {
    lassign [createSimpleDB] db lib
    set block [create1LevelBlock $db $lib [$db getChip]]
    set group [odb::dbGroup_create $block "group"]
    set domain [odb::dbRegion_create $block "domain"]
    odb::dbBox_create $domain 0 0 100 100
    set child [odb::dbGroup_create $block "child"]
    set master_mod [odb::dbModule_create $block "master_mod"]
    set parent_mod [odb::dbModule_create $block "parent_mod"]
    set i1 [odb::dbModInst_create $parent_mod $master_mod "i1"]
    set inst1 [$block findInst "inst"]
    set n1 [$block findNet "n1"]
    return [list $db $block $group $domain $i1 $inst1 $child $n1]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

proc test_default {} {
    lassign [setUp] db block group domain i1 inst1 child n1
    assert {[$group getName] == "group"}
    assert {[[$block findGroup "group"] getName] == "group"}
    assert {[llength [$block getGroups]] == 2}
    assert {[llength [$block getRegions]] == 1}
    set region_boundaries [$domain getBoundaries]
    assert {[llength region_boundaries] == 1}
    assert {[[lindex $region_boundaries 0] xMax] == 100}
    $group addModInst $i1
    assert {[[lindex [$group getModInsts] 0] getName] == "i1"}
    assert {[[$i1 getGroup] getName] == "group"}
    $group removeModInst $i1
    $group addInst $inst1
    assert {[[lindex [$group getInsts] 0] getName] == "inst"}
    assert {[[$inst1 getGroup] getName] == "group"}
    $group removeInst $inst1
    $group addGroup $child
    assert {[[lindex [$group getGroups] 0] getName] == "child"}
    assert {[[$child getParentGroup] getName] == "group"}
    $group removeGroup $child
    $group addPowerNet $n1
    assert {[[lindex [$group getPowerNets] 0] getName] == "n1"}
    $group addGroundNet $n1
    assert {[[lindex [$group getGroundNets] 0] getName] == "n1"}
    $group removeNet $n1
    assert {[$group getType] == "PHYSICAL_CLUSTER"}
    $group setType "VOLTAGE_DOMAIN"
    odb::dbGroup_destroy $group
    tearDown $db
}

test_default
puts "pass"
exit 0
