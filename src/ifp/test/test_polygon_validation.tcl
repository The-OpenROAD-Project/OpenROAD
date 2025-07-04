# Test validation of polygon coordinates
source "helpers.tcl"

# Setup
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# Test with odd number of coordinates (should fail)
puts "Testing with odd number of coordinates..."
catch {
    initialize_floorplan -die_polygon "0 0 0 150 200" -site FreePDK45_38x28_10R_NP_162NW_34O
} result

puts "Result: $result"

# Test with valid coordinates (should work)
puts "Testing with valid coordinates..."
initialize_floorplan -die_polygon "0 0 0 150 200 150 200 75 75 75 75 0" -site FreePDK45_38x28_10R_NP_162NW_34O
puts "Valid coordinates test passed!"
