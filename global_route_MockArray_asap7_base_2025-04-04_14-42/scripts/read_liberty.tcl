# To remove [WARNING STA-1212] from the logs for ASAP7.
# /OpenROAD-flow-scripts/flow/platforms/asap7/lib/asap7sc7p5t_SIMPLE_RVT_TT_nldm_211120.lib.gz line 13178, timing group from output port.
# Added following suppress_message
if {[env_var_equals PLATFORM asap7]} {
   suppress_message STA 1212
}

#Read Liberty
if {[env_var_exists_and_non_empty CORNERS]} {
  # corners
  define_corners {*}$::env(CORNERS)
  foreach corner $::env(CORNERS) {
    set LIBKEY "[string toupper $corner]_LIB_FILES"
    foreach libFile $::env($LIBKEY) {
    read_liberty -corner $corner $libFile
    }
    unset LIBKEY
  }
  unset corner
} else {
  ## no corner
  foreach libFile $::env(LIB_FILES) {
    read_liberty $libFile
  }
}

if {[env_var_equals PLATFORM asap7]} {
   unsuppress_message STA 1212
}
