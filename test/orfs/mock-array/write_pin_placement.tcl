source $::env(SCRIPTS_DIR)/global_place_skip_io.tcl

log_cmd place_pins \
  -hor_layers $::env(IO_PLACER_H) \
  -ver_layers $::env(IO_PLACER_V) \
  {*}[env_var_or_empty PLACE_PINS_ARGS]

write_pin_placement [file join $::env(WORK_HOME) "io-placement.tcl"]
