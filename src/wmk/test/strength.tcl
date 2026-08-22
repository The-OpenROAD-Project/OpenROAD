# The wrong-way cost multiplier round-trips through the router configuration.
source "helpers.tcl"
puts "default [get_routing_watermark_strength]"
set_routing_watermark_strength 8
puts "after set 8 [get_routing_watermark_strength]"
set_routing_watermark_strength 100
puts "after set 100 [get_routing_watermark_strength]"
