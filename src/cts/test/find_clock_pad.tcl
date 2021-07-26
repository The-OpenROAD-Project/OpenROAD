#Unit test to find clocks from pads. Also checks if the number of clocks found by TritonCTS is correct.

#This unit test was created with the help of James Cherry. The script this test is based on can be found in dbSta/test.

# find_clks from input port thru pad
read_lef Nangate45/Nangate45.lef
read_lef pad.lef
read_liberty Nangate45/Nangate45_typ.lib
read_liberty pad.lib
read_def find_clock_pad.def

create_clock -name clk -period 10 clk1

set_wire_rc -clock -layer metal5

clock_tree_synthesis -buf_list "BUF_X1"
