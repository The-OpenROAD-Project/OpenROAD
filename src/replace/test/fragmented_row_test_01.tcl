set design gcd
set lib_dir library/nangate45/
set design_dir design/nangate45/${design}

# Import LEF/DEF files
read_lef ${lib_dir}/NangateOpenCellLibrary.lef
read_def ${design_dir}/${design}_fragmented_row.def


# timing-driven parameters
read_liberty ${lib_dir}/NangateOpenCellLibrary_typical.lib
read_sdc ${design_dir}/${design}.sdc

global_placement -verbose 3 -skip_initial_place -timing_driven -wire_res 16 -wire_cap 0.23e-15

exit
