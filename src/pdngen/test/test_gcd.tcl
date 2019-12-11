read_lef ./nangate45/NangateOpenCellLibrary.mod.lef
read_def ./gcd/floorplan.def
run_pdngen ./gcd/PDN.cfg
write_def test_gcd.def
set f [open test_gcd.def]
fcopy $f stdout
close $f
exit
