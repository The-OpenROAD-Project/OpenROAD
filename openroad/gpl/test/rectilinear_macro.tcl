# fixed L-shaped block whose notch holds placement sites
source helpers.tcl
set test_name rectilinear_macro
read_lef ./nangate45.lef
read_lef ./$test_name.lef
read_def ./$test_name.def

global_placement -density 0.8
source report_hpwl.tcl
