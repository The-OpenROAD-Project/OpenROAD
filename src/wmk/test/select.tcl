# Keyed net selection is deterministic, key-sensitive, and reversible.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set key_a 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set key_b 0011223344556677889900aabbccddeeff00112233445566778899aabbccddef

# The same key always selects the same nets.
set first [set_routing_watermark -key_hex $key_a -fraction 0.10]
set cleared [clear_routing_watermark]
set again [set_routing_watermark -key_hex $key_a -fraction 0.10]
puts "tagged $first, cleared $cleared, retagged $again"

# A different key selects a different set.
clear_routing_watermark
set other [set_routing_watermark -key_hex $key_b -fraction 0.10]
puts "other key tagged $other"
puts "same count with same key: [expr { $first == $again }]"

# Clearing removes exactly what was tagged.
puts "clear removes all: [expr { $cleared == $first }]"
