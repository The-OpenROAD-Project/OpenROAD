# -buffer_cell arg checks
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def reg2.def

catch { buffer_ports -buffer_cell xxx/yyy } result
puts $result
catch { buffer_ports -buffer_cell Nangate_typ/yyy } result
puts $result
catch { buffer_ports -buffer_cell [get_lib_cell xxx/yyy] } result
puts $result
catch { buffer_ports -buffer_cell Nangate_typ/NOR2_X1 } result
puts $result
buffer_ports -buffer_cell Nangate_typ/BUF_X2
buffer_ports -buffer_cell [get_lib_cell Nangate_typ/BUF_X2]
