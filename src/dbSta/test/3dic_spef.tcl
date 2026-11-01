# Cross-die parasitics. One bond net arrives as three SPEF pieces -- chipA's
# wiring (2 fF), the die-to-die bond resistor (2 kohm), and chipB's wiring
# (100 fF). read_spef must merge them onto the chip-net so chipA's driver sees
# the far die's load through the bond; without that it sees only its own 2 fF
# and the path delay collapses from ~0.41 ns to ~0.15 ns.
#
# The bonds SPEF is top-scope and must be read first: a top-scope read replaces
# the net's parasitic network, while a -path read merges into it.
#
# The hierarchical_pin report at the end is a regression guard: it walks
# term -> pin -> net, which must ascend to the chip-net. If it ever resolves
# back to the die-side net the walk becomes a self-loop and this crashes.
source "helpers.tcl"

read_3dbx 3dic_cross.3dbx

create_clock -name clk -period 1.0 [get_pins -of_objects [get_nets clk_top]]

read_spef 3dic_spef.bonds.spef
read_spef -path chipA 3dic_spef.a.spef
read_spef -path chipB 3dic_spef.b.spef

report_parasitic_annotation
report_checks -path_delay max -group_path_count 1 -fields {cap hierarchical_pin}
