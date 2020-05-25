# -buffer_cell arg checks
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg2.def

catch { buffer_ports -buffer_cell xxx/yyy } result
puts $result
catch { buffer_ports -buffer_cell NangateOpenCellLibrary/yyy } result
puts $result
catch { buffer_ports -buffer_cell [get_lib_cell xxx/yyy] } result
puts $result
catch { buffer_ports -buffer_cell NangateOpenCellLibrary/NOR2_X1 } result
puts $result
buffer_ports -buffer_cell NangateOpenCellLibrary/BUF_X2
buffer_ports -buffer_cell [get_lib_cell NangateOpenCellLibrary/BUF_X2]
