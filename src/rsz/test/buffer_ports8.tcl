# buffer output with assign between output ports
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog buffer_ports8.v
link_design top

initialize_floorplan -site $site \
  -die_area {0 0 100 100} \
  -core_area {10 10 90 90}

source $tracks_file

place_pins -random -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer

global_placement

buffer_ports -inputs -outputs

set def_file [make_result_file buffer_ports8.def]
write_def $def_file
diff_files buffer_ports1.defok $def_file
