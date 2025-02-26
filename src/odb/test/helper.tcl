# Converted from helper.py

proc createSimpleDB {{use_default_db 0}} {
    if {$use_default_db} {
        set db [ord::get_db]
    } else {
        set db [odb::dbDatabase_create]
    }
    set tech [odb::dbTech_create $db "simple_tech"]
    set L1 [odb::dbTechLayer_create $tech "L1" "ROUTING"]
    set lib [odb::dbLib_create $db "lib" $tech "/"]
    set chip [odb::dbChip_create $db]
    set and2 [createMaster2X1 $lib "and2" 1000 1000 "a" "b" "o"]
    set or2 [createMaster2X1 $lib "or2" 500 500 "a" "b" "o"]
    return [list $db $lib]
}

proc createMultiLayerDB {} {
    set db [odb::dbDatabase_create]
    set tech [odb::dbTech_create $db "multi_tech"]

    set m1 [odb::dbTechLayer_create $tech "M1" "ROUTING"]
    $m1 setWidth 2000
    set m2 [odb::dbTechLayer_create $tech "M2" "ROUTING"]
    $m2 setWidth 2000
    set m3 [odb::dbTechLayer_create $tech "M3" "ROUTING"]
    $m3 setWidth 2000

    set v12 [odb::dbTechVia_create $tech "VIA12"]
    odb::dbBox_create $v12 $m1 0 0 2000 2000
    odb::dbBox_create $v12 $m2 0 0 2000 2000

    set v23 [odb::dbTechVia_create $tech "VIA23"]
    odb::dbBox_create $v23 $m2 0 0 2000 2000
    odb::dbBox_create $v23 $m3 0 0 2000 2000

    return [list $db $tech $m1 $m2 $m3 $v12 $v23]
}

# logical expr OUT = (IN1&IN2)
#
#     (n1)   +-----
# IN1--------|a    \    (n3)
#     (n2)   | (i1)o|-----------OUT
# IN2--------|b    /
#            +-----
proc create1LevelBlock {db lib parent} {
    set blockName "1LevelBlock"
    set block [odb::dbBlock_create $parent $blockName [$lib getTech] ","]
    # Creating Master and2 and instance inst
    set and2 [$lib findMaster "and2"]
    set inst [odb::dbInst_create $block $and2 "inst"]
    # creating our nets
    set n1 [odb::dbNet_create $block "n1"]
    set n2 [odb::dbNet_create $block "n2"]
    set n3 [odb::dbNet_create $block "n3"]
    set IN1 [odb::dbBTerm_create $n1 "IN1"]
    $IN1 setIoType "INPUT"
    set IN2 [odb::dbBTerm_create $n2 "IN2"]
    $IN2 setIoType "INPUT"
    set OUT [odb::dbBTerm_create $n3 "OUT"]
    $OUT setIoType "OUTPUT"
    # connecting nets
    set a [$inst findITerm "a"]
    $a connect $n1
    set b [$inst findITerm "b"]
    $b connect $n2
    set o [$inst findITerm "o"]
    $o connect $n3
    return $block
}


# logical expr OUT = (IN1&IN2) | (IN3&IN4)
#     (n1)   +-----
# IN1--------|a    \    (n5)
#     (n2)   | (i1)o|-----------+
# IN2--------|b    /            |       +-------
#            +-----             +--------\a     \    (n7)
#                                         ) (i3)o|---------------OUT
#     (n3)   +-----             +--------/b     /
# IN3--------|a    \    (n6)    |       +-------
#     (n4)   | (i2)o|-----------+
# IN4--------|b    /
#            +-----
proc create2LevelBlock { db lib parent } {
    set blockName "2LevelBlock"
    set block [odb::dbBlock_create $parent $blockName [$lib getTech] ","]

    set and2 [$lib findMaster "and2"]
    set or2 [$lib findMaster "or2"]
    # creating instances
    set i1 [odb::dbInst_create $block $and2 "i1"]
    set i2 [odb::dbInst_create $block $and2 "i2"]
    set i3 [odb::dbInst_create $block $or2 "i3"]
    # creating nets and block terms
    set n1 [odb::dbNet_create $block "n1"]
    set n2 [odb::dbNet_create $block "n2"]
    set n3 [odb::dbNet_create $block "n3"]
    set n4 [odb::dbNet_create $block "n4"]
    set n5 [odb::dbNet_create $block "n5"]
    set n6 [odb::dbNet_create $block "n6"]
    set n7 [odb::dbNet_create $block "n7"]

    set IN1 [odb::dbBTerm_create $n1 "IN1"]
    $IN1 setIoType "INPUT"
    set IN2 [odb::dbBTerm_create $n2 "IN2"]
    $IN2 setIoType "INPUT"
    set IN3 [odb::dbBTerm_create $n3 "IN3"]
    $IN3 setIoType "INPUT"
    set IN4 [odb::dbBTerm_create $n4 "IN4"]
    $IN4 setIoType "INPUT"
    set OUT [odb::dbBTerm_create $n7 "OUT"]
    $OUT setIoType "OUTPUT"
    # connecting $nets
    set i1_a [$i1 findITerm "a"]
    $i1_a connect $n1
    set i1_b [$i1 findITerm "b"]
    $i1_b connect $n2
    set i1_o [$i1 findITerm "o"]
    $i1_o connect $n5

    set i2_a [$i2 findITerm "a"]
    $i2_a connect $n3
    set i2_b [$i2 findITerm "b"]
    $i2_b connect $n4
    set i2_o [$i2 findITerm "o"]
    $i2_o connect $n6

    set i3_a [$i3 findITerm "a"]
    $i3_a connect $n5
    set i3_b [$i3 findITerm "b"]
    $i3_b connect $n6
    set i3_o [$i3 findITerm "o"]
    $i3_o connect $n7

    set P1 [odb::dbBPin_create $IN1]
    set P2 [odb::dbBPin_create $IN2]
    set P3 [odb::dbBPin_create $IN3]
    set P4 [odb::dbBPin_create $IN4]
    set P5 [odb::dbBPin_create $OUT]

    return $block
}

#  +-----
#  |in1   \
#      out|
#  |in2  /
#  +-----
proc createMaster2X1 {lib name width height in1 in2 out} {
    set master [odb::dbMaster_create $lib $name]
    $master setWidth $width
    $master setHeight $height
    $master setType "CORE"
    odb::dbMTerm_create $master $in1 "INPUT"
    odb::dbMTerm_create $master $in2 "INPUT"
    odb::dbMTerm_create $master $out "OUTPUT"
    $master setFrozen
    return $master
}

proc createMaster3X1 {lib name width height in1 in2 in3 out} {
    set master [odb::dbMaster_create $lib $name]
    $master setWidth $width
    $master setHeight $height
    $master setType "CORE"
    odb::dbMTerm_create $master $in1 "INPUT"
    odb::dbMTerm_create $master $in2 "INPUT"
    odb::dbMTerm_create $master $in3 "INPUT"
    odb::dbMTerm_create $master $out "OUTPUT"
    $master setFrozen
    return $master
}

#
# Assertion helper
#
proc assert { condition { message "assertion failed" } } {
    # Try to evaluate the condition as an expression.
    # This will work for numeric comparisons (e.g., $x == 5)
    if {![catch {uplevel 1 expr $condition}]} {
        # If the expression evaluates successfully and returns false, raise an error
        if {![uplevel 1 expr $condition]} {
            error $message
        }
    } else {
        # If the condition involves string comparison, use string equal or eq.
        if {[string equal $condition ""]} {
            error $message
        }
    }
}

#
# Assertion helper for strings
#
proc assertStringEq { val expected_val { message "assertion failed" } } {
    if {![string equal $val $expected_val]} {
        error $message
    }
}

#
# Assertion helper for strings
#
proc assertStringNotEq { val expected_val { message "assertion failed" } } {
    if {[string equal $val $expected_val]} {
        error $message
    }
}

#
# Assertion helper for checking that object is null
#
proc assertObjIsNull { val { message "assertion failed" } } {
    if {![string equal $val "NULL"]} {
        error $message
    }
}
    
#
# Assertion helper for checking that object is non-null
#
proc assertObjIsNotNull { val { message "assertion failed" } } {
    if {[string equal $val "NULL"]} {
        error $message
    }
}

#
# List comparison helper, since struct::list isn't available
# returns true if two lists are the same
#
proc lequal {l1 l2} {
    foreach elem $l1 {
        if { $elem ni $l2 } {
            return false
        }
    }
    foreach elem $l2 {
        if { $elem ni $l1 } {
            return false
        }
    }
    return true
}
