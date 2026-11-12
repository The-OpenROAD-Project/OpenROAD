# Padding check against a fixed block that is off the row/site grid (#12038)
source "helpers.tcl"
source "offrow_block.tcl"
read_lef offrow_block.lef
read_def check_offrow_block_padding.def

# ram at y=3.6 is 0.8 um into row 2 and covers rows 2-5 (top 7.9 um).
offrow_scenario "cell in row 6 above the block" \
  {2000 3600} {4000 8400} R0 pass
offrow_scenario "cell overlapping the block in row 5" \
  {2000 3600} {4000 7000} MX fail

set_placement_padding -masters INV -left 1
offrow_scenario "padded cell next to the block's bottom slice in row 2" \
  {2000 3600} {12000 2800} R0 fail
# ram on a row but 0.05 um off the site grid: right edge at 12.05 um
offrow_scenario "padded cell next to an off-site block" \
  {2050 2800} {12200 2800} R0 fail
offrow_scenario "padded cell one site clear of an off-site block" \
  {2050 2800} {12400 2800} R0 pass

exit_summary
