
# read the skywater LEF
read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/AC_unpadded/merged_unpadded.lef

# read the striVe_soc DEF
read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/AC_unpadded/Antenna_Checking/striVe_soc_s2/striVe_soc.def

# ARC API - will generate a report on the chip's nets and gates
check_antenna
