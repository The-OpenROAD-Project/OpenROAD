set ::env(VERILOG_FILES) $::env(RESULTS_DIR)/1_synth.rtlil

source $::env(SCRIPTS_DIR)/synth_preamble.tcl

hierarchy -check -top $::env(DESIGN_NAME)

if { [env_var_equals SYNTH_GUT 1] } {
  # /deletes all cells at the top level, which will quickly optimize away
  # everything else, including macros.
  delete $::env(DESIGN_NAME)/c:*
}

if {[env_var_exists_and_non_empty SYNTH_KEEP_MODULES]} {
  foreach module $::env(SYNTH_KEEP_MODULES) {
    select -module $module
    setattr -mod -set keep_hierarchy 1
    select -clear
  }
}

set synth_full_args $::env(SYNTH_ARGS)
if {[env_var_exists_and_non_empty SYNTH_OPERATIONS_ARGS]} {
  set synth_full_args [concat $synth_full_args $::env(SYNTH_OPERATIONS_ARGS)]
} else {
  set synth_full_args [concat $synth_full_args "-extra-map $::env(FLOW_HOME)/platforms/common/lcu_kogge_stone.v"]
}

if {![env_var_equals SYNTH_HIERARCHICAL 1]} {
  # Perform standard coarse-level synthesis script, flatten right away
  # (-flatten part of $synth_args per default)
  synth -run :fine {*}$synth_full_args
} else {
  # Perform standard coarse-level synthesis script,
  # defer flattening until we have decided what hierarchy to keep
  synth -run :fine

  if {[env_var_exists_and_non_empty SYNTH_MINIMUM_KEEP_SIZE]} {
    set ungroup_threshold $::env(SYNTH_MINIMUM_KEEP_SIZE)
    puts "Keep modules above estimated size of $ungroup_threshold gate equivalents"

    convert_liberty_areas
    keep_hierarchy -min_cost $ungroup_threshold
  } else {
    keep_hierarchy
  }

  # Re-run coarse-level script, this time do pass -flatten
  synth -run coarse:fine {*}$synth_full_args
}

json -o $::env(RESULTS_DIR)/mem.json
# Run report and check here so as to fail early if this synthesis run is doomed
exec -- python3 $::env(SCRIPTS_DIR)/mem_dump.py --max-bits $::env(SYNTH_MEMORY_MAX_BITS) $::env(RESULTS_DIR)/mem.json

if {![env_var_exists_and_non_empty SYNTH_WRAPPED_OPERATORS]} {
  synth -top $::env(DESIGN_NAME) -run fine: {*}$synth_full_args
} else {
  source $::env(SCRIPTS_DIR)/synth_wrap_operators.tcl
}

# Get rid of indigestibles
chformal -remove
delete t:\$print

# rename registers to have the verilog register name in its name
# of the form \regName$_DFF_P_. We should fix yosys to make it the reg name.
# At least this is predictable.
renames -wire

# Optimize the design
opt -purge

# Technology mapping of adders
if {[env_var_exists_and_non_empty ADDER_MAP_FILE]} {
  # extract the full adders
  extract_fa
  # map full adders
  techmap -map $::env(ADDER_MAP_FILE)
  techmap
  # Quick optimization
  opt -fast -purge
}

# Technology mapping of latches
if {[env_var_exists_and_non_empty LATCH_MAP_FILE]} {
  techmap -map $::env(LATCH_MAP_FILE)
}

set dfflibmap_args ""
foreach cell $::env(DONT_USE_CELLS) {
  lappend dfflibmap_args -dont_use $cell
}

# Technology mapping of flip-flops
# dfflibmap only supports one liberty file
if {[env_var_exists_and_non_empty DFF_LIB_FILE]} {
  dfflibmap -liberty $::env(DFF_LIB_FILE) {*}$dfflibmap_args
} else {
  dfflibmap -liberty $::env(DONT_USE_SC_LIB) {*}$dfflibmap_args
}
opt

if {![env_var_exists_and_non_empty SYNTH_WRAPPED_OPERATORS]} {
  log_cmd abc {*}$abc_args
} else {
  scratchpad -set abc9.script scripts/abc_speed_gia_only.script
  # crop out -script from arguments
  set abc_args [lrange $abc_args 2 end]
  log_cmd abc_new {*}$abc_args
  delete {t:$specify*}
}

# Replace undef values with defined constants
setundef -zero

# Splitting nets resolves unwanted compound assign statements in netlist (assign {..} = {..})
splitnets

# Remove unused cells and wires
opt_clean -purge

# Technology mapping of constant hi- and/or lo-drivers
hilomap -singleton \
        -hicell {*}$::env(TIEHI_CELL_AND_PORT) \
        -locell {*}$::env(TIELO_CELL_AND_PORT)

# Insert buffer cells for pass through wires
insbuf -buf {*}$::env(MIN_BUF_CELL_AND_PORTS)

# Reports
tee -o $::env(REPORTS_DIR)/synth_check.txt check

tee -o $::env(REPORTS_DIR)/synth_stat.txt stat {*}$stat_libs

# check the design is composed exclusively of target cells, and check for other problems
if {![env_var_exists_and_non_empty SYNTH_WRAPPED_OPERATORS]} {
  check -assert -mapped
} else {
  # Wrapped operator synthesis leaves around $buf cells which `check -mapped`
  # gets confused by, once Yosys#4931 is merged we can remove this branch and
  # always run `check -assert -mapped`
  check -assert
}

# Write synthesized design
write_verilog -nohex -nodec $::env(RESULTS_DIR)/1_1_yosys.v
# One day a more sophisticated synthesis will write out a modified
# .sdc file after synthesis. For now, just copy the input .sdc file,
# making synthesis more consistent with other stages.
log_cmd exec cp $::env(SDC_FILE) $::env(RESULTS_DIR)/1_synth.sdc
