# connect/disconnect pin/port
read_lef example1.lef
read_def example1.def
read_liberty example1_slow.lib

disconnect_pin in1 r1/D
disconnect_pin in1 [get_ports in1]
report_net -connections in1

connect_pin in1 r1/D
connect_pin in1 [get_ports in1]
report_net -connections in1
