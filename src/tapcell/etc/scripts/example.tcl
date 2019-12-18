read_lef input.lef
read_def input.def

tapcell -endcap_cpp "1" -distance "25" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

write_def output.def
