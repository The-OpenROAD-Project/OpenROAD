source "helpers.tcl"

read_lef "data/gscl45nm.lef"
read_def "data/design58.def"

set via [odb::dbVia_create [ord::get_db_block] "testing_via"]

foreach viagen [[ord::get_db_tech] getViaGenerateRules] {
    if {[$viagen getName] == "M2_M1"} {
        break
    }
}

$via setViaGenerateRule $viagen

set params [$via getViaParams]
$params setBottomLayer [[ord::get_db_tech] findLayer metal1]
$params setCutLayer [[ord::get_db_tech] findLayer via1]
$params setTopLayer [[ord::get_db_tech] findLayer metal2]
$params setXCutSize 130
$params setYCutSize 130
$params setXCutSpacing 390
$params setYCutSpacing 390
$params setNumCutRows 5
$params setNumCutCols 5

$via setViaParams $params

set swire [[[ord::get_db_block] findNet VDD] getSWires]

set via0 [odb::dbSBox_create $swire $via 2000 2000 IOWIRE]
set via1 [odb::dbSBox_create $swire $via 5000 5000 IOWIRE]

puts [llength [$via1 smashVia]]

odb::dbSBox_destroy $via1

set out_def [make_result_file "smash_vias.def"]
write_def $out_def

diff_files $out_def "smash_vias.defok"

puts "pass"
exit 0
