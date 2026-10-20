# Check micron/DBU boundaries, option forwarding, and read-only preflight outputs.
source "helpers.tcl"

check_error "missing design" { check_macro_placement } MPL-0001

set db [ord::get_db]
$db setDbuPerMicron 2000
set tech [odb::dbTech_create $db preflight_tech]
$tech setLefUnits 2000
set lib [odb::dbLib_create $db preflight_lib $tech /]
$lib setLefUnits 2000
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip preflight]
$block setDefUnits 2000
set rect [odb::Rect]
$rect init 0 0 80000 100000
$block setDieArea $rect
$rect init 20250 40500 44250 64500
$block setCoreArea $rect
set master [odb::dbMaster_create $lib macro_master]
$master setType BLOCK
$master setWidth 10000
$master setHeight 10000
$master setFrozen
set macro_a [odb::dbInst_create $block $master macro_a]
odb::dbInst_create $block $master macro_b

# The 5 x 5 um macros exactly fill a 10 x 5 um fence. Moving any fence
# coordinate inward by one DBU (0.0005 um) must fail the area check.
proc run_fence_check { fence args } {
  lassign $fence lx ly ux uy
  return [check_macro_placement -fence_lx $lx -fence_ly $ly \
    -fence_ux $ux -fence_uy $uy {*}$args]
}
set exact_fence {10.125 20.25 20.125 25.25}
set work_dir [file join [make_result_dir] preflight_artifacts]
file mkdir $work_dir
cd $work_dir
set artifacts_before [lsort [glob -nocomplain * .*]]
mpl::set_macro_placement_file unexpected-placement.tcl
mpl::set_min_channel 1.0 2.0

check "default preflight" { check_macro_placement } true
check "exact fractional-origin fence" { run_fence_check $exact_fence } true
foreach {coordinate fence} {
  lx {10.1255 20.25 20.125 25.25}
  ly {10.125 20.2505 20.125 25.25}
  ux {10.125 20.25 20.1245 25.25}
  uy {10.125 20.25 20.125 25.2495}
} {
  check_error "one DBU inward at $coordinate" { run_fence_check $fence } MPL-0065
}
foreach {axis fence} {
  x {22.125 20.25 32.125 25.25}
  y {10.125 32.25 20.125 37.25}
  xy {22.125 32.25 32.125 37.25}
} {
  check_error "fence touches core on $axis" { run_fence_check $fence } MPL-0068
}
check "one DBU channel rounds down to zero halo" {
  run_fence_check $exact_fence -min_channel_size 0.0005
} true
check_error "two DBU scalar channel consumes area" {
  run_fence_check $exact_fence -min_channel_size 0.001
} MPL-0065
check_error "two DBU horizontal channel consumes area" {
  run_fence_check $exact_fence -min_channel_size {0.001 0}
} MPL-0065
check_error "two DBU vertical channel consumes area" {
  run_fence_check $exact_fence -min_channel_size {0 0.001}
} MPL-0065
check "pin-aware channels trim a pinless macro" {
  run_fence_check $exact_fence -min_channel_size 0.001 -pin_aware_channels
} true
check_error "channel cardinality" {
  check_macro_placement -min_channel_size {1 2 3}
} MPL-0080
check_error "pin-aware channel requirement" {
  check_macro_placement -pin_aware_channels
} MPL-0079
check "no reports or placement files on success or error" {
  lsort [glob -nocomplain * .*]
} $artifacts_before

# Observe the previously configured channel through a subsequent operation.
# Preflight's per-call overrides must not replace this 1 x 2 um setting.
$macro_a setLocation 24250 44500
$macro_a setPlacementStatus FIRM
block_macro_channels
check "one fixed-macro blockage" { llength [$block getBlockages] } 1
set box [[lindex [$block getBlockages] 0] getBBox]
check "configured channel survives successful and failing preflights" {
  list [$box xMin] [$box yMin] [$box xMax] [$box yMax]
} {23250 42500 35250 56500}

exit_summary
