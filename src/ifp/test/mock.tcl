source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def mock.def
initialize_floorplan -utilization 30 \
  -aspect_ratio 0.5 \
  -core_space 1 