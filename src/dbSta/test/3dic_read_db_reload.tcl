# Restore the 3DIC database written by 3dic_read_db.tcl in a fresh process.
source "helpers.tcl"

read_liberty ../../odb/test/Nangate45/Nangate45_typ.lib
read_db [make_result_file 3dic_read_db.odb]

# Structural model restored.
report_3dic_summary

# Timing network restored: the same cross-chiplet constrained path forms.
create_clock -name clk -period 1.0 \
  [get_pins -of_objects [get_nets clk_top]]
report_checks -path_delay max
