#package require control
#control::control assert enabled 1
#
# Converted from test_destroy.py
# TclTest isn't available so just calling tests sequentially
#

# destroying   dbInst,  dbNet,  dbBTerm,  dbBlock,  dbBPin,  dbWire,  dbCapNode,
# dbCcSeg,  dbLib,  dbSWire,  dbObstruction,  dbRegion
# TODO         dbRSeg,  dbRCSeg,  dbRow,  dbTarget,  dbTech

source "helper.tcl"

proc setup_regions {block} {
    set parentRegion [odb::dbRegion_create $block "parentRegion"]
    return $parentRegion
}

proc setUp {} {
    lassign [createSimpleDB] db lib
    set parentBlock [odb::dbBlock_create [$db getChip] "Parent"]
    set block [create2LevelBlock $db $lib $parentBlock]
    set i1 [$block findInst "i1"]
    set i2 [$block findInst "i2"]
    set i3 [$block findInst "i3"]
    set n1 [[$i1 findITerm "a"] getNet]
    set n2 [[$i1 findITerm "b"] getNet]
    set n3 [[$i2 findITerm "a"] getNet]
    set n4 [[$i2 findITerm "b"] getNet]
    set n5 [[$i3 findITerm "a"] getNet]
    set n6 [[$i3 findITerm "b"] getNet]
    set n7 [[$i3 findITerm "o"] getNet]
    return [list $db $block $parentBlock $i1 $n1 $n2 $n4 $n5 $n7]
}

proc tearDown {db} {
    odb::dbDatabase_destroy $db
}

proc test_destroy_net {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    odb::dbNet_destroy $n1
    # check for Inst
    assertObjIsNull [[$i1 findITerm "a"] getNet] "inst term i1/a has non null net"
    # check for Iterms
    foreach iterm [$block getITerms] {
        if {[string equal [$iterm getNet] "NULL"]} {
            assertStringEq [[$iterm getInst] getName] "i1"
            assertStringEq [[$iterm getMTerm] getName] "a"
        } else {
            assertStringNotEq [[$iterm getNet] getName] "n1"
        }
    }
    # check for block and BTerms
    set nets [$block getNets]
    foreach net $nets {
        assertStringNotEq [$net getName] "n1" "found net n1"
    }
    set bterms [$block getBTerms]
    assert {[llength $bterms] == 4} [format "Found more than four bterms: " [llength $bterms]]
    foreach bterm $bterms {
        assertStringNotEq [$bterm getName] "IN1"
        assertStringNotEq [[$bterm getNet] getName] "n1"
    }
    assertObjIsNull [$block findBTerm "IN1"] "Bterm IN1 was found"
    assertObjIsNull [$block findNet "n1"] "Net n1 was found"
    tearDown $db
}

proc test_destroy_inst {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    odb::dbInst_destroy $i1
    # check for block
    assertObjIsNull [$block findInst "i1"]
    foreach inst [$block getInsts] {
        assertStringNotEq [$inst getName] "i1"
    }
    assert {[llength [$block getITerms]] == 6} [format "Inst term count is not 6: %d" [llength [$block getITerms]]]
    # check for Iterms
    foreach iterm [$block getITerms] {
        assert {[lsearch ["n1" "n2"] [[$iterm getNet] getName]] == -1}
        assertStringNotEq [[$iterm getInst] getName] "i1"
    }
    # check for BTERMS
    set IN1 [$block findBTerm "IN1"]
    assertObjIsNull [$IN1 getITerm] "Bterm IN1's inst term isn't null"
    set IN2 [$block findBTerm "IN2"]
    assertObjIsNull [$IN2 getITerm] "Bterm IN2's inst term isn't null"
    # check for nets
    assert {[$n1 getITermCount] == 0} [format "Net %s inst term count is not 0: %d" [$n1 getName] [$n1 getITermCount]]
    assert {[$n2 getITermCount] == 0} [format "Net %s inst term count is not 0: %d" [$n2 getName] [$n2 getITermCount]]
    assert {[$n5 getITermCount] == 1} [format "Net %s inst term count is not 1: %d" [$n5 getName] [$n5 getITermCount]]
    assertStringNotEq [[[lindex [$n5 getITerms] 0] getInst] getName] "i1"
    tearDown $db
}

proc test_destroy_bterm {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set IN1 [$block findBTerm "IN1"]
    odb::dbBTerm_destroy $IN1
    # check for block and BTerms
    assertObjIsNull [$block findBTerm "IN1"]
    set bterms [$block getBTerms]
    assert {[llength $bterms] == 4}
    foreach bterm $bterms {
        assertStringNotEq [$bterm getName] "IN1"
    }
    # check for n1
    assert {[$n1 getBTermCount] == 0}
    assert {[llength [$n1 getBTerms]] == 0}
    tearDown $db
}

proc test_destroy_block {} {
    # creating a child block to parent block
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set other_block [create1LevelBlock $db [lindex [$db getLibs] 0] $parentBlock]
    assert {[llength [$parentBlock getChildren]] == 2}
    odb::dbBlock_destroy $other_block
    assert {[llength [$parentBlock getChildren]] == 1}
    odb::dbBlock_destroy $parentBlock
    tearDown $db
}

proc test_destroy_bpin {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set IN1 [$block findBTerm "IN1"]
    assert {[llength [$IN1 getBPins]] == 1}
    set P1 [lindex [$IN1 getBPins] 0]
    odb::dbBPin_destroy $P1
    assert {[llength [$IN1 getBPins]] == 0}
    tearDown $db
}

proc test_create_destroy_wire {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set w [odb::dbWire_create $n7]
    assertObjIsNotNull [$n7 getWire] "wire is null"
    odb::dbWire_destroy $w
    assertObjIsNull [$n7 getWire] "wire is not null" 
    tearDown $db
}

proc test_destroy_capnode {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set node2 [odb::dbCapNode_create $n2 0 false]
    set node1 [odb::dbCapNode_create $n1 1 false]
    set seg [odb::dbCCSeg_create $node1 $node2]
    assert {[$n1 getCapNodeCount] == 1}
    assert {[$n1 getCcCount] == 1}
    odb::dbCapNode_destroy $node1
    assert {[$n1 getCapNodeCount] == 0}
    assert {[$n1 getCcCount] == 0}
    tearDown $db
}

proc test_destroy_ccseg {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set node2 [odb::dbCapNode_create $n2 0 false]
    set node1 [odb::dbCapNode_create $n1 1 false]
    set ccseg [odb::dbCCSeg_create $node1 $node2]
    assert {[llength [$node1 getCCSegs]] == 1}
    assert {[llength [$block getCCSegs]] == 1}
    assert {[$n1 getCcCount == 1]}
    odb::dbCCSeg_destroy $ccseg
    assert {[lequal [$node1 getCCSegs] []]}
    assert {[lequal [$block getCCSegs] []]}
    assert {[$n1 getCcCount] == 0}
    tearDown $db
}

proc test_destroy_lib {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set lib [lindex [$db getLibs] 0]
    odb::dbLib_destroy $lib
    assert {[llength [$db getLibs]] == 0}
    tearDown $db
}

proc test_destroy_swire {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set swire [odb::dbSWire_create $n4 "ROUTED"]
    assert {[llength [$n4 getSWires]] == 1} "swire wasn't created"
    assertStringEq [[$swire getNet] getName] [$n4 getName] [format "swire nets don't have same name: %s %s" [[$swire getNet] getName] [$n4 getName]]
    odb::dbSWire_destroy $swire
    assert {[llength [$n4 getSWires]] == 0} "swire wasn't destroyed"
    tearDown $db
}

proc test_destroy_obstruction {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set tech [[lindex [$db getLibs] 0] getTech]
    set L1 [lindex [$tech getLayers] 0]
    set obst [odb::dbObstruction_create $block $L1 0 0 1000 1000]
    assert {[llength [$block getObstructions]] == 1}
    odb::dbObstruction_destroy $obst
    assert {[llength [$block getObstructions]] == 0}
    tearDown $db
}

proc test_create_regions {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set parentRegion [setup_regions $block]
    assertObjIsNull [$i1 getRegion] "region is not null"
    set region_list [$block getRegions]
    assert {[llength $region_list] == 1} [format "region list length is not 1: %d" [llength $region_list]]
    assertStringEq [[lindex $region_list 0] getName] [$parentRegion getName] [format "first region name doesn't match parent: %s %s" [[lindex $region_list 0] getName] [$parentRegion getName]]
    tearDown $db
}

proc test_destroy_region_parent {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set parentRegion [setup_regions $block]
    odb::dbRegion_destroy $parentRegion
    assert {[llength [$block getRegions] == 0}
    tearDown $db
}

proc test_destroy_trackgrid {} {
    lassign [setUp] db block parentBlock i1 n1 n2 n4 n5 n7
    set tech [[lindex [$db getLibs] 0] getTech]
    set L1 [$tech findLayer "L1"]
    set grid [odb::dbTrackGrid_create $block $L1]
    assertObjIsNull [odb::dbTrackGrid_create $block $L1] "able to create already existing track grid"
    odb::dbTrackGrid_destroy $grid
    assertObjIsNotNull [odb::dbTrackGrid_create $block $L1] "not able to create track grid"
    tearDown $db
}

test_destroy_net
test_destroy_inst
test_destroy_bterm
test_destroy_block
test_destroy_bpin
test_create_destroy_wire
test_destroy_capnode
test_destroy_ccseg
test_destroy_lib
test_destroy_swire
test_destroy_obstruction
test_create_regions
test_destroy_region_parent
test_destroy_trackgrid
puts "pass"
exit 0
