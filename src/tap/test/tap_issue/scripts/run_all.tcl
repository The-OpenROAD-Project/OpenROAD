# Performs RTL-to-GDS in a single script
# No checkpointing is performed

set standalone 0

# Synthesis
source -verbose $::env(SCRIPTS_DIR)/yosys.tcl

# Read process / design
# Read lef
read_lef $::env(TECH_LEF)
read_lef $::env(SC_LEF)
if {[info exist ::env(ADDITIONAL_LEFS)]} {
    foreach lef $::env(ADDITIONAL_LEFS) {
      read_lef $lef
    }
}

# Read liberty files
foreach libFile $::env(LIB_FILES) {
  read_liberty $libFile
}

read_verilog $::env(RESULTS_DIR)/1_1_yosys.v
link_design $::env(DESIGN_NAME)
read_sdc $::env(SDC_FILE)
if [file exists $::env(PLATFORM_DIR)/derate.tcl] {
  source $::env(PLATFORM_DIR)/derate.tcl
}

if [file exists $::env(PLATFORM_DIR)/setRC.tcl] {
  source $::env(PLATFORM_DIR)/setRC.tcl
}

# Floorplan
source -verbose $::env(SCRIPTS_DIR)/floorplan.tcl
source -verbose $::env(SCRIPTS_DIR)/io_placement_random.tcl
source -verbose $::env(SCRIPTS_DIR)/tdms_place.tcl
source -verbose $::env(SCRIPTS_DIR)/macro_place.tcl
source -verbose $::env(SCRIPTS_DIR)/tapcell.tcl
source -verbose $::env(SCRIPTS_DIR)/pdn.tcl

# Place
source -verbose $::env(SCRIPTS_DIR)/global_place.tcl
source -verbose $::env(SCRIPTS_DIR)/io_placement.tcl
source -verbose $::env(SCRIPTS_DIR)/resize.tcl
source -verbose $::env(SCRIPTS_DIR)/detail_place.tcl

# CTS
source -verbose $::env(SCRIPTS_DIR)/cts.tcl
source -verbose $::env(SCRIPTS_DIR)/fillcell.tcl

# Route
source -verbose $::env(SCRIPTS_DIR)/global_route.tcl
source -verbose $::env(SCRIPTS_DIR)/detail_route.tcl

# Finishing
if {[info exists ::env(USE_FILL)]} {
  source -verbose $::env(SCRIPTS_DIR)/density_fill.tcl
}

# Finishing
source -verbose $::env(SCRIPTS_DIR)/final_report.tcl
source -verbose $::env(SCRIPTS_DIR)/klayout.tcl

exit
