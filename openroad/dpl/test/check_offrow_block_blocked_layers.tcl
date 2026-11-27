# Blocked layers check of a fixed block that is off the row/site grid
source "helpers.tcl"
source "offrow_block.tcl"
read_lef offrow_block.lef
read_def check_offrow_block_blocked_layers.def

# ram's connected metal1 pin makes it use metal1/metal2, so it must not sit
# on the vertical metal2 VDD stripes:
#   stripe A at x=7.0 um, y 8.5-9.7 um (row 6 only)
#   stripe B at x=12.1 um, y 2.9-4.1 um (row 2, site 60)
offrow_scenario "block below stripe A, top at 7.9 um" \
  {2000 3600} {16000 0} R0 pass
offrow_scenario "block bottom at 9.2 um, over stripe A" \
  {2000 9200} {16000 0} R0 fail
offrow_scenario "block on row 6, over stripe A" \
  {2000 8400} {16000 0} R0 fail
offrow_scenario "off-site block, right edge 12.15 um over stripe B" \
  {2150 2800} {16000 0} R0 fail
offrow_scenario "off-site block, right edge 11.95 um clear of stripe B" \
  {1950 2800} {16000 0} R0 pass

exit_summary
