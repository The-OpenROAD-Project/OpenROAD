# Read standard cells and macros as blackbox inputs
# These libs have their dont_use properties set accordingly
read_liberty -overwrite -setattr liberty_cell -lib {*}$::env(DONT_USE_LIBS)
read_liberty -overwrite -setattr liberty_cell \
  -unit_delay -wb -ignore_miss_func -ignore_buses {*}$::env(DONT_USE_LIBS)
