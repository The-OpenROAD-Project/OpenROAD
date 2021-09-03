source "helpers.tcl"


set db [ord::get_db]
read_lef "data/gscl45nm.lef"
set lib [$db findLib gscl45nm]
read_def "data/design.def"
set chip [$db getChip]
if {$chip == "NULL"} {
    puts "FAIL: Read DEF Failed"
    exit 1
}
set block [$chip getBlock]
set net [odb::dbNet_create $block "w1"]
$net setSigType "POWER"
set swire [odb::dbSWire_create $net "ROUTED"]
if {[$swire getNet] != "$net"} {
    puts "FAIL: Net error"
    exit 1
}
set site [lindex [$lib getSites] 0]
set row [odb::dbRow_create $block "row0" $site 0 0 "RO" "HORIZONTAL" 1 10]
if {$row == "NULL"} {
    puts "FAIL: row error"
    exit 1
}
puts [$net getConstName]
puts [$row getConstName]
puts [[$swire getNet] getConstName]
puts "pass"
exit 0
