# findClkNets(clock)
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def find_clks2.def

create_clock -period 10 clk1
create_clock -period 10 clk2
create_clock -period 10 clk3
foreach clk [sta::find_all_clk_nets] {
    puts [$clk getName]
}
