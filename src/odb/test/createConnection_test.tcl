source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/parser_test.def"
set chip [$db getChip]
set block [$chip getBlock]
set tech [$db getTech]

odb::createPGpins $block $tech VDD 3 0

odb::createConnection [ord::get_db_block] "pg_VDD" "_d0_" "D"

puts "pass"
exit 0