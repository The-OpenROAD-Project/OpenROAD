# SetupLegacyMtPolicy move-tracker-style reporting smoke test
set ::env(RSZ_POLICY) legacy_mt
set move_tracker_level 2

source report_move_tracker.tcl
unset -nocomplain ::env(RSZ_POLICY)
