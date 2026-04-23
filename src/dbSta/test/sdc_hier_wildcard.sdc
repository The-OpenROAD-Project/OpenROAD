# Toy SDC that exercises dbSdcNetwork::findInstancesMatching1 prefix pruning.
create_clock -period 10 -name clk1 [get_ports clk1]
create_clock -period 20 -name clk2 [get_ports clk2]

# Literal prefix + trailing wildcard: prefix pruning anchors at b1.
set_false_path -from [get_cells b1/*]

# Literal prefix + partial wildcard in last segment.
set_false_path -from [get_cells b2/u*]

# Leading wildcard: no prefix to prune, full DFS fallback path.
set_false_path -from [get_cells */r1]

# Fully literal: direct-lookup fast path (never reaches the DFS).
set_false_path -from [get_cells b1/r1]
