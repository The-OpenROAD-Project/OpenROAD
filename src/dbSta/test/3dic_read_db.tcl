source "helpers.tcl"

# write_db/read_db round-trip of a 3DIC design. The restore path arrives via
# postReadDb (not postRead3Dbx); the 3DIC timing network must be initialized
# there too, so the restored design times identically.
read_3dbx 3dic_cross.3dbx

set db_file [make_result_file 3dic_read_db.odb]
write_db $db_file
read_db $db_file

# Structural model restored.
report_3dic_summary

# Timing network restored: the same cross-chiplet constrained path forms.
create_clock -name clk -period 1.0 \
  [get_pins -of_objects [get_nets clk_top]]
report_checks -path_delay max
