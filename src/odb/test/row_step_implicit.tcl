# Regression for DEF ROW DO/BY with no STEP (OpenROAD #11706).
source "helpers.tcl"

read_lef "data/gscl45nm.lef"
read_def "data/row_step_implicit.def"

set block [ord::get_db_block]
check "row count" {llength [$block getRows]} 5

array set rows {}
foreach row [$block getRows] {
  set rows([$row getName]) $row
}

# {name direction count spacing {xMin yMin xMax yMax}}
foreach {name direction count spacing expected_bbox} {
  h_implicit HORIZONTAL 10 760 {10000 10000 17600 14940}
  h_explicit HORIZONTAL 10 900 {10000 20000 18860 24940}
  v_implicit VERTICAL 10 4940 {40000 10000 40760 59400}
  v_explicit VERTICAL 10 6000 {50000 10000 50760 68940}
  single_explicit HORIZONTAL 1 0 {60000 10000 60760 14940}
} {
  if {![info exists rows($name)]} {
    error "Missing DEF row $name"
  }
  set row $rows($name)
  check "$name direction" {$row getDirection} $direction
  check "$name site count" {$row getSiteCount} $count
  check "$name spacing" {$row getSpacing} $spacing

  set bbox [$row getBBox]
  check "$name bbox" {
    list [$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]
  } $expected_bbox
}

exit_summary
