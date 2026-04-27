# Regression for dbSdcNetwork literal SDC pin lookup when an instance in
# the hierarchy has a name containing the path divider (a Verilog escaped
# identifier like "\i/sub").  Before the full-path -> Instance map fallback
# in findInstancesMatching1, get_pins on a literal pattern that traverses
# such an instance silently dropped (STA-0363) because the hierarchy walker
# in findInstance splits the embedded '/' as a hierarchy separator.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog sdc_modinst_slash.v
link_design -hier top

# Confirm the modInst really is stored with the embedded '/' (escaped).
set block [ord::get_db_block]
set top_mod [$block getTopModule]
foreach child [$top_mod getModInsts] {
  puts "modinst name: '[$child getName]'"
}

# Literal get_pins through the embedded-slash modInst.  This is what was
# missed before the fix.
set pins [get_pins i/sub/buf1/A]
foreach p $pins {
  puts "get_pins i/sub/buf1/A -> [get_property $p full_name]"
}

# Apply a real SDC constraint and confirm it sticks (no STA-0363 warning).
set_disable_timing [get_pins i/sub/buf1/A]
report_disabled_edges
