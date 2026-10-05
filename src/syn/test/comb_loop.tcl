# Combinational loops (e.g. inferred latches) are rejected by synthesize
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib
read_liberty comb_loop_cells.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_lef comb_loop_cells.lef

sv_elaborate --std=1364-2005 --top false_loop comb_loop.v
synthesize -reduce_name_loss
puts "false_loop synthesized"

sv_elaborate --std=1364-2005 --top ram_loop comb_loop.v
synthesize -reduce_name_loss
puts "ram_loop synthesized"

sv_elaborate --std=1364-2005 --top dual_no_loop comb_loop.v
synthesize -reduce_name_loss
puts "dual_no_loop synthesized"

sv_elaborate --std=1364-2005 --top half_loop comb_loop.v
catch { synthesize -reduce_name_loss } error
puts $error

sv_elaborate --std=1364-2005 --top latch_clock_gate comb_loop.v
catch { synthesize -reduce_name_loss } error
puts $error
