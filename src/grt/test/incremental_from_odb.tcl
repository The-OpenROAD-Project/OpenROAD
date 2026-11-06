# Incremental global routing on a design read from odb starts from the global
# routing stored in its guides: a no-op update reports their demand instead of
# an empty grid.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_db "sky130hs/gcd_grt.db"

global_route -start_incremental
global_route -end_incremental -verbose
