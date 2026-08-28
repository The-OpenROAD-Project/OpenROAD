# Without -sdc, read_db ignores the constraints stored in the .odb, so
# scripts that read their own .sdc see exactly the behavior they saw before.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_db sdc_in_db.odb

puts "clocks: [get_clocks *]"
