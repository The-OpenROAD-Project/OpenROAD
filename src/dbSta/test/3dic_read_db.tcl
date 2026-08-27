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

# Top-level ports restored: the port names and directions are stored as
# properties on the chip nets and must survive the .odb round trip.
puts "ports: [lsort [lmap p [get_ports *] { get_full_name $p }]]"
puts "clk_top direction: [get_property [get_ports clk_top] direction]"
puts "raw_top direction: [get_property [get_ports raw_top] direction]"

# Timing network restored, with the clock anchored on the top-level port:
# it must reach the flops of both dies from outside.
create_clock -name clk -period 1.0 [get_ports clk_top]
set_input_delay 0.1 -clock clk [get_ports in_top]
set_output_delay 0.1 -clock clk [get_ports out_top]
report_checks -path_delay max -group_path_count 3
