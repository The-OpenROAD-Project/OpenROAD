# Test estimate_path_resistance command
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty Nangate45/Nangate45_typ.lib
read_def "gcd.def"

set_routing_layers -signal metal2-metal10

global_route

# Test 1: Basic usage with ITerms
puts "Test 1: Basic usage with ITerms"
set res [estimate_path_resistance _858_/D _762_/Z -verbose]
puts "Resistance: $res"

# Test 2: Usage with layers
puts "\nTest 2: Usage with layers"
set res [estimate_path_resistance _858_/D _762_/Z -layer1 metal2 -layer2 metal3 -verbose]
puts "Resistance: $res"

# Test 3: Usage with BTerms
puts "\nTest 3: Usage with BTerms"

puts "Estimating resistance between BTerm clk and ITerm _858_/CK"
set res [estimate_path_resistance clk _858_/CK -verbose]
puts "Resistance: $res"

# Test 4: Error handling - Invalid pin
puts "\nTest 4: Error handling - Invalid pin"
catch { estimate_path_resistance "invalid_pin" _762_/Z } err
puts "Expected error: $err"

# Test 5: Error handling - Missing layer
puts "\nTest 5: Error handling - Missing layer"
catch { estimate_path_resistance _858_/D _762_/Z -layer1 metal2 } err
puts "Expected error: $err"
