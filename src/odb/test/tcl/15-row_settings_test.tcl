set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
set lib [odb::read_lef $db [file join $data_dir "Nangate45" "NangateOpenCellLibrary.mod.lef"]]
odb::read_def $db [file join $data_dir "gcd" "floorplan.def"]
set chip [$db getChip]

set block [$chip getBlock]
set tech [[$block getDataBase] getTech]
set lib [lindex [concat {*}[[$block getDataBase] getLibs]] 0]
set site [lindex [set sites [$lib getSites]] 0]
set rt [odb::dbRow_create $block ROW_test $site 0 380 "MX" "HORIZONTAL" 420 380]

check "row name" {$rt getName} ROW_test
check "row origin" {$rt getOrigin} "0 380"
check "row site" {[$rt getSite] getName} [$site getName]
check "row direction" {$rt getDirection} "HORIZONTAL"
check "row orientation" {$rt getOrient} "MX"
check "row spacing" {$rt getSpacing} 380
check "row site count" {$rt getSiteCount} 420

set rt1 [odb::dbRow_create $block ROW_test $site 0 380 "R0" "HORIZONTAL" 420 380]

check "row direction" {$rt1 getDirection} "HORIZONTAL"
check "row orientation" {$rt1 getOrient} "R0"

set rt2 [odb::dbRow_create $block ROW_test $site 0 380 "R0" "VERTICAL" 420 380]

check "row direction" {$rt2 getDirection} "VERTICAL"
check "row orientation" {$rt2 getOrient} "R0"

exit_summary
