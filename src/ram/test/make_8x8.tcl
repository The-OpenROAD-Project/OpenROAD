source ./helpers.tcl
source /che/OpenROAD-flow-scripts/tools/OpenROAD/src/ram/src/ram.tcl


#load /che/OpenROAD-flow-scripts/tools/OpenROAD/build/src/ram/ram.a
#export PATH=/che/OpenROAD-flow-scripts/tools/OpenROAD/build:$PATH
#which openroad
#make
#make install
#source ./env.sh
#openroad make_8x8.tcl



read_liberty /che/OpenROAD-flow-scripts/tools/OpenROAD/src/ram/test/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef     /che/OpenROAD-flow-scripts/tools/OpenROAD/src/ram/test/sky130hd/sky130hd.tlef
read_lef     /che/OpenROAD-flow-scripts/tools/OpenROAD/src/ram/test/sky130hd/sky130_fd_sc_hd_merged.lef

generate_ram_netlist \
    -bytes_per_word 1 \
    -word_count 8 \
    -read_ports 2 \
    -storage_cell sky130_fd_sc_hd__dlxtp_1 \
    -tristate_cell sky130_fd_sc_hd__ebufn_2 \
    -inv_cell sky130_fd_sc_hd__inv_1


ord::design_created
set def_file [make_result_file make_8x8.def]
write_def 
#diff_files make_8x8.defok 


