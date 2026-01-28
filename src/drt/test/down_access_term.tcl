# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "down_access_term.def"
read_guides "down_access_term.guide"

set def_file [make_result_file down_access_term.def]

# set layer metal5 to be rect only to enforce access from below
set tech [ord::get_db_tech]
set metal5 [$tech findLayer met5]
$metal5 setRectOnly True

detailed_route -verbose 0

write_def $def_file
diff_files down_access_term.defok $def_file
