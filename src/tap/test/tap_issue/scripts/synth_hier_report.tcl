yosys -import

if {[info exist ::env(CACHED_NETLIST)]} {
  exec cp $::env(CACHED_NETLIST) $::env(RESULTS_DIR)/1_1_yosys.v
  if {[info exist ::env(CACHED_REPORTS)]} {
    exec cp {*}$::env(CACHED_REPORTS) $::env(REPORTS_DIR)/.
  }
  exit
}

# Setup verilog include directories
set vIdirsArgs ""
if {[info exist ::env(VERILOG_INCLUDE_DIRS)]} {
  foreach dir $::env(VERILOG_INCLUDE_DIRS) {
    lappend vIdirsArgs "-I$dir"
  }
  set vIdirsArgs [join $vIdirsArgs]
}


# Read verilog files
foreach file $::env(VERILOG_FILES) {
  read_verilog -defer -sv {*}$vIdirsArgs $file
}




# Read standard cells and macros as blackbox inputs
# These libs have their dont_use properties set accordingly
read_liberty -lib {*}$::env(DONT_USE_LIBS)

# Apply toplevel parameters (if exist)
if {[info exist ::env(VERILOG_TOP_PARAMS)]} {
  dict for {key value} $::env(VERILOG_TOP_PARAMS) {
    chparam -set $key $value $::env(DESIGN_NAME)
  }
}

# Read platform specific mapfile for OPENROAD_CLKGATE cells
if {[info exist ::env(CLKGATE_MAP_FILE)]} {
  read_verilog -defer $::env(CLKGATE_MAP_FILE)
}

# Mark modules to keep from getting removed in flattening
if {[info exist ::env(PRESERVE_CELLS)]} {
  # Expand hierarchy since verilog was read in with -defer
  hierarchy -check -top $::env(DESIGN_NAME)
  foreach cell $::env(PRESERVE_CELLS) {
    select -module $cell
    setattr -mod -set keep_hierarchy 1
    select -clear
  }
}



if {[info exist ::env(BLOCKS)]} {
  hierarchy -check -top $::env(DESIGN_NAME)
  foreach block $::env(BLOCKS) {
    blackbox $block
    puts "blackboxing $block"
  }
}

set constr [open $::env(OBJECTS_DIR)/abc.constr w]
puts $constr "set_driving_cell $::env(ABC_DRIVER_CELL)"
puts $constr "set_load $::env(ABC_LOAD_IN_FF)"
close $constr

# Hierarchical synthesis
synth  -top $::env(DESIGN_NAME)
techmap -map $::env(ADDER_MAP_FILE)
techmap
if {[info exist ::env(DFF_LIB_FILE)]} {
  dfflibmap -liberty $::env(DFF_LIB_FILE)
} else {
  dfflibmap -liberty $::env(DONT_USE_SC_LIB)
}
abc -liberty $::env(DONT_USE_SC_LIB) \
    -constr $::env(OBJECTS_DIR)/abc.constr

# Create argument list for stat
set stat_libs ""
foreach lib $::env(DONT_USE_LIBS) {
  append stat_libs "-liberty $lib "
}
tee -o $::env(REPORTS_DIR)/synth_hier_stat.txt stat {*}$stat_libs

if { [info exist ::env(REPORTS_DIR)] && [file isfile $::env(REPORTS_DIR)/synth_hier_stat.txt] } {
  set ungroup_threshold 0
  if { [info exist ::env(MAX_UNGROUP_SIZE)] && $::env(MAX_UNGROUP_SIZE) > 0 } {
    set ungroup_threshold $::env(MAX_UNGROUP_SIZE)
    puts "Ungroup modules of size $ungroup_threshold"
  }
  hierarchy -check -top $::env(DESIGN_NAME)
  set fptr [open $::env(REPORTS_DIR)/synth_hier_stat.txt r]
  set contents [read -nonewline $fptr]
  close $fptr
  set split_cont [split $contents "\n"]
  set out_script_ptr [open $::env(OBJECTS_DIR)/mark_hier_stop_modules.tcl w]
  puts $out_script_ptr "hierarchy -check -top $::env(DESIGN_NAME)"
  foreach line $split_cont {
    if {[regexp { +Chip area for module '\\(\S+)': (.*)} $line -> module_name area]} {
        if {[expr $area > $ungroup_threshold]} {
           puts "Preserving hierarchical module: $module_name"
           puts $out_script_ptr "select -module $module_name"
           puts $out_script_ptr "setattr -mod -set keep_hierarchy 1"
           puts $out_script_ptr "select -clear"
        }
    }
  }
  close $out_script_ptr
}

