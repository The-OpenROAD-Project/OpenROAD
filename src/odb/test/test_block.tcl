#package require control
#control::control assert enabled 1
#
# Converted from test_block.py
# TclTest isn't available so just calling tests sequentially
#
source "helper.tcl"

proc placeInst {inst x y} {
    $inst setLocation $x $y
    $inst setPlacementStatus "PLACED"
}

proc placeBPin {bpin layer x1 y1 x2 y2} {
    odb::dbBox_create $bpin $layer $x1 $y1 $x2 $y2
    $bpin setPlacementStatus "PLACED"
}

# Set up data
proc setUp {} {
    lassign [createSimpleDB] db lib
    set parentBlock [odb::dbBlock_create [$db getChip] "Parent"]
    set block [create2LevelBlock $db $lib $parentBlock]
    $block setCornerCount 4
    set extcornerblock [$block createExtCornerBlock 1]
    odb::dbTechNonDefaultRule_create $block "non_default_1"
    set parentRegion [odb::dbRegion_create $block "parentRegion"]
    return [list $db $lib $block $parentBlock]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

proc test_find {} {
    lassign [setUp] db lib block parentBlock
    # bterm
    assertStringEq [[$block findBTerm "IN1"] getName] "IN1"
    assertObjIsNull [$block findBTerm "in1"]
    # child
    assertStringEq [[$parentBlock findChild "2LevelBlock"] getName] "2LevelBlock"
    assertObjIsNull [$parentBlock findChild "1LevelBlock"]
    # inst
    assertStringEq [[$block findInst "i3"] getName] "i3"
    assertObjIsNull [$parentBlock findInst "i3"]
    # net
    assertStringEq [[$block findNet "n2"] getName] "n2"
    assertObjIsNull [$block findNet "a"]
    # iterm
    set iterm [$block findITerm "i1,o"]
    assertObjIsNotNull $iterm "iterm i1.o is null"
    assertStringEq [[$iterm getInst] getName] "i1"
    assertStringEq [[$iterm getMTerm] getName] "o"
    assertObjIsNull [$block findITerm "i1\o"]
    # extcornerblock
    assertStringEq [[$block findExtCornerBlock 1] getName] "extCornerBlock__1"
    assertObjIsNull [$block findExtCornerBlock 0]
    # nondefaultrule
    assertStringEq [[$block findNonDefaultRule "non_default_1"] getName] "non_default_1"
    assertObjIsNull [$block findNonDefaultRule "non_default_2"]
    # region
    assertStringEq [[$block findRegion "parentRegion"] getName] "parentRegion"
    tearDown $db
}

proc check_box_rect {block min_x min_y max_x max_y} {
    set box [$block getBBox]
    assert {[$box xMin] == $min_x} [format "bbox xMin doesn't match: %d %d" [$box xMin] $min_x]
    assert {[$box xMax] == $max_x} [format "bbox xMax doesn't match: %d %d" [$box xMax] $max_x]
    assert {[$box yMin] == $min_y} [format "bbox yMin doesn't match: %d %d" [$box yMin] $min_y]
    assert {[$box yMax] == $max_y} [format "bbox yMax doesn't match: %d %d" [$box yMax] $max_y]
}

proc block_placement {block lib test_num flag} {
    if {($flag && $test_num == 1) || (!$flag && $test_num >= 1)} {
        if {$flag} {
            puts "here"
        }
        placeInst [$block findInst "i1"] 0 3000
        placeInst [$block findInst "i2"] -1000 0
        placeInst [$block findInst "i3"] 2000 -1000
    }
    if {($flag && $test_num == 2) || (!$flag && $test_num >= 2)} {
        placeBPin [lindex [[$block findBTerm "OUT"] getBPins] 0] \
                  [[$lib getTech] findLayer "L1"] 2500 -1000 2550 -950
    }
    if {($flag && $test_num == 3) || (!$flag && $test_num >= 3)} {
        odb::dbObstruction_create $block [[$lib getTech] findLayer "L1"] \
            -1500 0 -1580 50
    }
    if {($flag && $test_num == 4) || (!$flag && $test_num >= 4)} {
        set n_s [odb::dbNet_create $block "n_s"]
        set swire [odb::dbSWire_create $n_s "NONE"]
        odb::dbSBox_create $swire [[$lib getTech] findLayer "L1"] \
            0 4000 100 4100 "NONE"
    }
    if {($flag && $test_num == 5) || (!$flag && $test_num >= 5)} {
        # TODO ADD WIRE
    }
}

proc test_bbox0 {} {
    lassign [setUp] db lib block parentBlock
    set box [$block getBBox]
    check_box_rect $block 0 0 0 0
    tearDown $db
}

proc test_bbox1 {} {
    lassign [setUp] db lib block parentBlock
    set box [$block getBBox]
    block_placement $block $lib 1 false
    check_box_rect $block -1000 -1000 2500 4000
    tearDown $db
}

proc test_bbox2 {} {
    lassign [setUp] db lib block parentBlock
    set box [$block getBBox]
    block_placement $block $lib 2 false
    check_box_rect $block -1000 -1000 2550 4000
    tearDown $db
}

proc test_bbox3 {} {
    lassign [setUp] db lib block parentBlock
    #    block_placement $block 2 false
    #    set box [$block getBBox]
    #    block_placement $block 3 true
    set layer [[$lib getTech] findLayer "L1"]
    placeInst [$block findInst "i1"] 0 3000
    placeInst [$block findInst "i2"] -1000 0
    placeInst [$block findInst "i3"] 2000 -1000
    placeBPin [lindex [[$block findBTerm "OUT"] getBPins] 0] \
        $layer 2500 -1000 2550 -950
    set box [$block getBBox]
    odb::dbObstruction_create $block $layer -1500 0 -1580 50
    check_box_rect $block -1580 -1000 2550 4000
    tearDown $db
}

proc test_bbox4 {} {
    lassign [setUp] db lib block parentBlock
    set box [$block getBBox]
    block_placement $block $lib 4 false
    check_box_rect $block -1580 -1000 2550 4100
    tearDown $db
}

test_find
test_bbox0
test_bbox1
test_bbox2
test_bbox3
test_bbox4
puts "pass"
exit 0
