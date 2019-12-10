read_lef ./nangate45/NangateOpenCellLibrary.mod.lef
read_def ./gcd/floorplan.def
run_pdngen -verbose ./gcd/PDN.cfg
