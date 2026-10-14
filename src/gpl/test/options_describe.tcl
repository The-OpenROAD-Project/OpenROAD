# The help of an options struct comes from its C++ declaration, including the
# embedded repair_design and global_route options.
describe_options global_placement
puts "---- markdown"
describe_options global_placement -markdown
puts "---- defaults"
puts [default_options repair_design]
puts "---- report"
report_options global_route -options {congestion_iterations 10 grid_origin {5 5}}
