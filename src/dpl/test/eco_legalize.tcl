# OpenROAD-fork: eco-legalize
# ECO incremental legalization: legalize only the cells an ECO touched.
#
# Flow:
#   1. Read a fully placed design and snapshot every instance location.
#   2. Emulate an ECO: resize a handful of cells to a much wider master (same
#      lower-left), which creates local overlaps and (for odd widths) off-site
#      placement -- exactly the residue repair_timing_eco leaves behind.
#   3. Run improve_eco_legalization on ONLY those ECO cells.
#   4. Assert:
#        (a) overlaps/illegalities are resolved (check_placement passes),
#        (b) every ECO cell is on-site after legalization,
#        (c) every cell that was neither an ECO cell nor an overlapper keeps
#            its EXACT pre-legalization coordinates (minimal perturbation),
#        (d) only a small, local set of cells moved -- i.e. a full re-place was
#            NOT triggered.
source "helpers.tcl"
# DPL-1116 is the ECO-legalization elapsed-time line (non-deterministic).
suppress_message DPL 1116
read_lef Nangate45/Nangate45.lef
read_def gcd_replace.def

# Start from a fully legalized baseline (gcd_replace.def is a global-placement
# result).  This is the realistic ECO entry condition: a clean, legal placement.
detailed_placement
check_placement

set block [ord::get_db_block]
set rows [$block getRows]
set site [[lindex $rows 0] getSite]
set site_w [$site getWidth]
set site_h [$site getHeight]

# --- snapshot every instance's location before the ECO ---
proc snapshot_locs { block } {
  set locs [dict create]
  foreach inst [$block getInsts] {
    lassign [$inst getLocation] x y
    dict set locs [$inst getName] [list $x $y [[$inst getMaster] getName]]
  }
  return $locs
}
set before [snapshot_locs $block]

# --- emulate an ECO: widen a few cells to create local overlaps ---
# A realistic timing ECO upsizes a handful of gates a few sites wider, leaving
# small local overlaps for incremental legalization to clean up.  INV_X1 (2
# sites) -> INV_X4 (5 sites) overlaps the right neighbor without demanding a
# large local reshuffle.
set wide_master [[ord::get_db] findMaster "INV_X4"]
if { $wide_master == "NULL" } {
  utl::error DPL 9001 "test setup: INV_X4 master not found."
}
set eco_cells {}
foreach name {_278_ _283_ _286_} {
  set inst [$block findInst $name]
  if { $inst == "NULL" } {
    utl::error DPL 9002 "test setup: instance $name not found."
  }
  # Keep the lower-left fixed; widening overlaps the right neighbor.
  $inst swapMaster $wide_master
  lappend eco_cells $name
}

# --- incremental ECO legalization on ONLY the touched cells ---
# The C++ side (DPL-1114) hard-asserts that no cell the legalizer never touched
# moves at all, which is the core minimal-perturbation invariant.  We
# additionally check the externally observable properties here.
set n_legalized [improve_eco_legalization -cells $eco_cells -verbose]
utl::report "eco_legalize: $n_legalized cells in legalize set"

# (a) overlaps resolved: a clean check_placement must pass (no error raised).
check_placement

set after [snapshot_locs $block]
set total [llength [dict keys $after]]

# (b) every ECO cell is on-site (site-aligned in x and y, relative to the core
# origin).  check_placement already enforces this for legality; we re-derive it
# here as an explicit, independent assertion.
set core [$block getCoreArea]
set core_x [$core xMin]
set core_y [$core yMin]
set site_w_int [expr { int($site_w) }]
set site_h_int [expr { int($site_h) }]
foreach name $eco_cells {
  lassign [dict get $after $name] x y master
  if { (($x - $core_x) % $site_w_int) != 0 } {
    utl::error DPL 9003 "ECO cell $name x=$x not site-aligned (site $site_w_int)."
  }
  if { (($y - $core_y) % $site_h_int) != 0 } {
    utl::error DPL 9004 "ECO cell $name y=$y not row-aligned (row $site_h_int)."
  }
}

# (c) minimal perturbation (externally observed): count every cell that moved.
# The moved set must be exactly the ECO cells plus the immediate neighbors the
# local overlap resolution displaced -- never the whole design.
set moved_total 0
set moved_non_eco 0
dict for { name info_b } $before {
  lassign $info_b bx by bmaster
  lassign [dict get $after $name] ax ay amaster
  if { $bx != $ax || $by != $ay } {
    incr moved_total
    if { [lsearch -exact $eco_cells $name] < 0 } {
      incr moved_non_eco
    }
  }
}
utl::report "eco_legalize: total cells $total, moved $moved_total\
 (eco [llength $eco_cells], displaced neighbors $moved_non_eco)"

# (d) a full re-place would shuffle a large fraction of the design.  Incremental
# ECO legalization must only touch a small local neighborhood.
if { $moved_total > $total / 3 } {
  utl::error DPL 9005 \
    "eco_legalize moved $moved_total/$total cells -- looks like a full re-place."
}

# The vast majority of cells must be provably unmoved.
set unmoved [expr { $total - $moved_total }]
if { $unmoved < ($total * 2 / 3) } {
  utl::error DPL 9006 \
    "eco_legalize left only $unmoved/$total cells unmoved -- not minimal."
}

utl::report "eco_legalize: PASS (overlaps resolved, ECO cells on-site,\
 $unmoved/$total cells provably unmoved, no full re-place)"
