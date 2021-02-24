set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
set opendb_dir [file dirname $tests_dir]
source [file join $tcl_dir "test_helpers.tcl"]

# Open database, load lef and design

set db [odb::dbDatabase_create]
set lib [odb::read_lef $db [file join $data_dir "Nangate45" "NangateOpenCellLibrary.mod.lef"]]
odb::read_def $db [file join $data_dir "gcd" "floorplan.def"]
set chip [$db getChip]

# Block checks

set block [$chip getBlock]
set tech [$db getTech]

set via [odb::dbVia_create $block via1_960x340]

check "via name" {$via getName} via1_960x340
check "via params" {$via hasParams} 0

set params [$via getViaParams]

$params setBottomLayer [$tech findLayer metal1]
$params setCutLayer [$tech findLayer via1]
$params setTopLayer [$tech findLayer metal2]
$params setXCutSize 140
$params setYCutSize 140
$params setXCutSpacing 160
$params setYCutSpacing 160 
$params setXBottomEnclosure 110
$params setYBottomEnclosure 100
$params setXTopEnclosure 110
$params setYTopEnclosure 100
$params setNumCutRows 1
$params setNumCutCols 3

$via setViaParams $params

check "via now has params" {$via hasParams} 1

set p [$via getViaParams]

check "via bottom layer" {[$p getBottomLayer] getName} metal1
check "via top layer" {[$p getTopLayer] getName} metal2
check "via cut layer" {[$p getCutLayer] getName} via1
check "via cut size" {list [$p getXCutSize] [$p getYCutSize]} "140 140"
check "via cut spacing" {list [$p getXCutSpacing] [$p getYCutSpacing]} "160 160"
check "via bottom enclosure" {list [$p getXBottomEnclosure] [$p getYBottomEnclosure]} "110 100"
check "via top enclosure" {list [$p getXTopEnclosure] [$p getYTopEnclosure]} "110 100"
check "via rowcol" {list [$p getNumCutRows] [$p getNumCutCols]} "1 3"

set boxes [$via getBoxes]

set layer_boxes {}
foreach box $boxes {
    set layerName [[$box getTechLayer] getName]
    dict lappend layer_boxes $layerName $box
}

check "num metal1 shapes" {llength [dict get $layer_boxes metal1]} 1
check "num metal2 shapes" {llength [dict get $layer_boxes metal2]} 1
check "num via1 shapes" {llength [dict get $layer_boxes via1]} 3

set metal1_shape [lindex [dict get $layer_boxes metal1] 0]
set metal2_shape [lindex [dict get $layer_boxes metal2] 0]
set via1_shape [lindex [dict get $layer_boxes via1] 0]

set w_expected [expr 3 * 140 + (3 - 1) * 160 + 2 * 110]
set h_expected [expr 1 * 140 + (1 - 1) * 160 + 2 * 100]
check "metal1 size" {list [expr [$metal1_shape xMax] - [$metal1_shape xMin]] [expr [$metal1_shape yMax] - [$metal1_shape yMin]]} "$w_expected $h_expected" 

set w_expected [expr 3 * 140 + (3 - 1) * 160 + 2 * 110]
set h_expected [expr 1 * 140 + (1 - 1) * 160 + 2 * 100]
check "metal2 size" {list [expr [$metal2_shape xMax] - [$metal2_shape xMin]] [expr [$metal2_shape yMax] - [$metal2_shape yMin]]} "$w_expected $h_expected" 

set w_expected 140
set h_expected 140
check "via1 size" {list [expr [$via1_shape xMax] - [$via1_shape xMin]] [expr [$via1_shape yMax] - [$via1_shape yMin]]} "$w_expected $h_expected" 

exit_summary
