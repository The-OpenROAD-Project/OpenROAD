# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Export timing data as JSON for static HTML reports.
# Used by orfs_run targets to extract data from a built design.

source $::env(SCRIPTS_DIR)/open.tcl

set design ""
if { [info exists ::env(DESIGN_NAME)] } {
  set design $::env(DESIGN_NAME)
}
set stage ""
if { [info exists ::env(STAGE_NAME)] } {
  set stage $::env(STAGE_NAME)
}
set variant ""
if { [info exists ::env(VARIANT_NAME)] } {
  set variant $::env(VARIANT_NAME)
}

web_export_json -output $::env(OUTPUT) \
    -design $design -stage $stage -variant $variant
