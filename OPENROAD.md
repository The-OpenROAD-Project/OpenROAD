

# Put the local build directory at the front of PATH.
export PATH="/che/OpenROAD-flow-scripts/tools/OpenROAD/build/src:$PATH"

which openroad

./env.sh


read_liberty /che/OpenROAD-flow-scripts/tools/OpenROAD/src/ram/test/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef     /che/OpenROAD-flow-scripts/tools/OpenROAD/src/ram/test/sky130hd/sky130hd.tlef
read_lef     /che/OpenROAD-flow-scripts/tools/OpenROAD/src/ram/test/sky130hd/sky130_fd_sc_hd_merged.lef



generate_ram_netlist -bytes_per_word 1 -word_count 8 -read_ports 1
# Basic RAM generation
generate_ram_netlist -bytes_per_word 4 -word_count 32 -read_ports 1

# Enhanced DFFRAM-style memory
generate_dffram -bytes_per_word 4 -word_count 32 -memory_type 1RW -read_ports 1 -output_def ram32x32.def

# Generate standard RAM macro
generate_ram_macro -size 32x32 -memory_type 1RW -output_def ram32x32.def

# Generate register file
generate_register_file -word_count 32 -word_width 32 -output_def rf32x32.def

# 1. Launch OpenROAD
openroad

<!-- # 2. Read libraries
read_liberty /path/to/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef /path/to/sky130hd.tlef
read_lef /path/to/sky130_fd_sc_hd_merged.lef -->

# 3. Generate the RAM
generate_dffram -bytes_per_word 4 -word_count 32 -memory_type 1RW -read_ports 1 -output_def ram32x32.def

# 4. Write database if needed
write_db ram32x32.db










# Generate a standard 32-word x 32-bit RAM
generate_ram_netlist -bytes_per_word 4 -word_count 32 -memory_type 1RW -read_ports 1

generate_ram_netlist -bytes_per_word 4 -word_count 32 -read_ports 1

# Generate a larger RAM with optimized area
generate_ram_macro -size 128x32 -memory_type 1RW1R -output_def ram128.def

# Generate a register file
generate_register_file -word_count 32 -word_width 32




generate_ram_netlist \
    -bytes_per_word 1 \
    -word_count 8 \
    -read_ports 2 \
    -storage_cell sky130_fd_sc_hd__dlxtp_1 \
    -tristate_cell sky130_fd_sc_hd__ebufn_2 \
    -inv_cell sky130_fd_sc_hd__inv_1

generate_ram_netlist \
    -bytes_per_word 4 \
    -word_count 32 \
    -read_ports 1 \
    -storage_cell sky130_fd_sc_hd__dlxtp_1 \
    -tristate_cell sky130_fd_sc_hd__ebufn_2 \
    -inv_cell sky130_fd_sc_hd__inv_1

generate_ram_netlist \
    -bytes_per_word 4 \
    -word_count 8 \
    -read_ports 2 \
    -storage_cell sky130_fd_sc_hd__dlxtp_1 \
    -tristate_cell sky130_fd_sc_hd__ebufn_2 \
    -inv_cell sky130_fd_sc_hd__inv_1

ord::design_created
set def_file [make_result_file make_8x8.def]
write_def $def_file
diff_files make_8x8.defok $def_file


write_def ram8x8.def
read_def ram8x8.def

/che/OpenROAD-flow-scripts/tools/install/OpenROAD/bin/openroad








