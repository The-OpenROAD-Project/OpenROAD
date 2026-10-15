# The help of a command's switches comes from its C++ options struct,
# including the embedded repair_design and global_route options.
help global_placement
puts "---- describe"
describe_options global_placement
puts "---- markdown"
describe_options global_placement -markdown
puts "---- report"
report_options global_route -congestion_iterations 10 -grid_origin {5 5} -use_cugr
