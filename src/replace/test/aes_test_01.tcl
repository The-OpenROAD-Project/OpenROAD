set design aes_cipher_top
set lib_dir library/nangate45/
set design_dir design/nangate45/${design}

puts "${design_dir}/${design}.def"

# Import LEF/DEF files
read_lef ${lib_dir}/NangateOpenCellLibrary.lef
read_def ${design_dir}/${design}.def


# timing-driven parameters
read_liberty ${lib_dir}/NangateOpenCellLibrary_typical.lib
read_sdc ${design_dir}/${design}.sdc

global_placement -verbose 5 -density 1.0 -init_density_penalty 0.0001 

write_def ${design}_output_mac.def

