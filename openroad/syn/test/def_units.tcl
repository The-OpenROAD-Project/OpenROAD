# The block synthesize exports carries the tech's DEF units, as read_verilog's
# does: write_def scales by them, and the default of 100 would round a
# 2000-unit Nangate45 design to 10 nm.
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

sv_elaborate --std=1364-2005 --top false_loop comb_loop.v
synthesize -reduce_name_loss

set tech [ord::get_db_tech]
set block [ord::get_db_block]
puts "LEF units [$tech getLefUnits], DEF units [$block getDefUnits]"
