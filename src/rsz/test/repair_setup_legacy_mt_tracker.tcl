# SetupLegacyMtPolicy move-tracker-style reporting smoke test
set move_tracker_level 2
set repair_args [list -policy "LEGACY_MT LAST_GASP"]

source report_move_tracker.tcl
