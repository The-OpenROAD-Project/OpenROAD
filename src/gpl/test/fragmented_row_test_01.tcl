set design gcd
set lib_dir library/nangate45/
set design_dir design/nangate45/${design}

# Import LEF/DEF files
read_lef ${lib_dir}/NangateOpenCellLibrary.lef
read_def ${design_dir}/${design}_fragmented_row.def


# timing-driven parameters
read_liberty ${lib_dir}/NangateOpenCellLibrary_typical.lib
read_sdc ${design_dir}/${design}.sdc

global_placement -timing_driven -verbose 3 -skip_initial_place
