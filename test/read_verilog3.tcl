# read_verilog 16b bus with lef/liberty
read_lef bus16.lef
read_liberty bus16.lib
read_verilog bus16.v
link_design top
report_instance -connections bus1
