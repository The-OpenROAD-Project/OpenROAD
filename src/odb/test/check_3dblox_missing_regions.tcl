source "helpers.tcl"
set db [ord::get_db]

read_3dbx data/missing_regions.3dbx
check_3dblox

# The missing regions issue should be reported under the "Connected regions" category
check "Missing regions marker generated" { get_3dblox_marker_count "Connection regions" } 1

exit_summary
