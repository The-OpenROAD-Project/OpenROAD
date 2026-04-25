source helpers.tcl
set test_name net_weight_negative
read_lef ./nangate45.lef
read_def ./simple01.def

# Negative Test 1: non-existent net
puts "=== Test 1: non-existent net ==="
if {[catch {set_net_weight nonexistent_net 5.0} err]} {
    puts "PASS: caught error: $err"
} else {
    error "FAIL: should have thrown error for non-existent net"
}

# Negative Test 2: zero weight
puts "=== Test 2: zero weight ==="
if {[catch {set_net_weight clk 0.0} err]} {
    puts "PASS: caught error for zero weight: $err"
} else {
    error "FAIL: should reject zero weight"
}

# Negative Test 3: negative weight
puts "=== Test 3: negative weight ==="
if {[catch {set_net_weight clk -1.0} err]} {
    puts "PASS: caught error for negative weight: $err"
} else {
    error "FAIL: should reject negative weight"
}

# Negative Test 4: unset non-existent net
puts "=== Test 4: unset non-existent net ==="
if {[catch {unset_net_weight nonexistent_net} err]} {
    puts "PASS: caught error: $err"
} else {
    error "FAIL: should have thrown error for non-existent net"
}

# Negative Test 5: wrong number of arguments
puts "=== Test 5: wrong arg count ==="
if {[catch {set_net_weight} err]} {
    puts "PASS: caught error for no args: $err"
} else {
    error "FAIL: should reject no args"
}

if {[catch {set_net_weight a b c} err]} {
    puts "PASS: caught error for 3 args: $err"
} else {
    error "FAIL: should reject 3 args"
}

# Negative Test 6: non-numeric weight
puts "=== Test 6: non-numeric weight ==="
if {[catch {set_net_weight clk abc} err]} {
    puts "PASS: caught error for non-numeric: $err"
} else {
    error "FAIL: should reject non-numeric weight"
}

# Negative Test 7: unset net that has no weight (should not crash)
puts "=== Test 7: unset net without weight ==="
set nets [odb::dbBlock_getNets [ord::get_db_block]]
set first_net [lindex $nets 0]
set net_name [$first_net getName]
# Don't set weight, just unset — should be no-op, no crash
unset_net_weight $net_name
puts "PASS: unset on net without weight (no-op)"

puts "=== All negative tests passed ==="
