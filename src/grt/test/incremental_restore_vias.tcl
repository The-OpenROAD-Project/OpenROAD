# Routes restored from DB guides during an incremental ECO must be
# electrically connected by construction (GlobalRouter::addImplicitVias).
#
# When guides do not carry explicit via info, boxToGlobalRouting leaves layer
# transitions implicit, so the per-net parasitic network splits into
# disconnected islands. The driver-reduced wire capacitance then collapses to
# zero (the orphaned cap is unreachable from the driver), even though the SPEF
# still dumps every node. addImplicitVias bridges the coincident adjacent-layer
# points so the restored route is a single connected network.
#
# This test forces the incremental single-net guide-restore path
# (loadRoutingFromDBGuides) via the ODB journal: deleting a net's guides inside
# an ECO and undoing it recreates the guides and fires the
# inDbNetPostGuideRestore callback, marking the net for route restoration.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd.def
read_guides est_rc3.guide
set_routing_layers -signal metal2-metal10

set block [ord::get_db_block]
set clk_net [$block findNet "clk"]

# Trigger the journal-driven guide restore for clk.
global_route -start_incremental
odb::dbDatabase_beginEco $block
foreach guide [$clk_net getGuides] {
  odb::dbGuide_destroy $guide
}
odb::dbDatabase_undoEco $block
global_route -end_incremental

# Estimate parasitics from the restored route and read the driver-reduced wire
# capacitance (this is what collapses to 0 when the network is fragmented).
estimate_parasitics -global_routing
with_output_to_variable report { report_net -digits 5 clk }
regexp {Wire capacitance:\s+([0-9.]+)} $report -> wire_cap

# Without addImplicitVias the restored clk network is fragmented and this is
# 0.0; with the fix the driver reaches the whole net (~17.3).
check "restored clk route is connected (non-zero wire capacitance)" {
  expr {$wire_cap > 1.0}
} 1

exit_summary
