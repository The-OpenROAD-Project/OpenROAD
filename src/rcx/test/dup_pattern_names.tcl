# bench_wires names each pattern after its widths and spacings rounded onto a
# coarse integer nm grid, so two spacing table entries that differ as doubles
# can still land on the same name. rcx used to generate the colliding pattern
# anyway and segfault on the null net odb returned for the duplicate name.
# See OpenROAD issue #7897.
source helpers.tcl

read_lef dup_pattern_names.tlef

bench_wires -len 100 -all

# Metal3 is the layer with the collision: SPACING and PITCH-WIDTH are one ULP
# apart as doubles and both round to 1800nm in the name. The pattern has to be
# generated exactly once, with all of its wires.
set block [ord::get_db_block]
puts "nets: [llength [$block getNets]]"
foreach id {1 2 3 4 5} {
  set net [$block findNet "R6_M3oM0_W2200W2200_S00000S01800_$id"]
  if { $net != "NULL" } {
    puts "R6_M3oM0_W2200W2200_S00000S01800_$id has [llength [$net getBTerms]] bterms"
  }
}
