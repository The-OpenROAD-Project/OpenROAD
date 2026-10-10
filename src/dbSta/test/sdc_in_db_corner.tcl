# Second process for the corner-scope refusal: runs write_db -sdc with a
# corner-scoped derate so the parent can read the refusal, which is a
# warning the parent's own log must not carry.
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog sdc_in_db.v
link_design top
create_clock -name clk -period 10 [get_ports clk1]
if { [info commands set_cmd_analysis_corner] == "" } {
  puts "no analysis corners in this build"
  exit 0
}
define_analysis_corner slow
set_cmd_analysis_corner slow
set_timing_derate -late 1.05
unset_cmd_analysis_corner
write_db -sdc $::env(SDC_IN_DB_ODB)
puts "after write_db, stored form: [ord::sdc_in_db_kind]"
