set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
set lib [odb::read_lef $db [file join $data_dir "gscl45nm.lef"]]
odb::read_def $db [file join $data_dir "design.def"]
set chip [$db getChip]
if {$chip == "NULL"} {
    puts "Read DEF Failed"
    exit 1
}
set block [$chip getBlock]
set net [odb::dbNet_create $block "w1"]
$net setSigType "POWER"
set swire [odb::dbSWire_create $net "ROUTED"]
if {[$swire getNet] != "$net"} {
    exit 1
}
set site [lindex [$lib getSites] 0]
set row [odb::dbRow_create $block "row0" $site 0 0 "RO" "HORIZONTAL" 1 10]
if {$row == "NULL"} {
    exit 1
}
puts [$net getConstName]
puts [$row getConstName]
puts [[$swire getNet] getConstName]
exit 0
