source $::env(SCRIPTS_DIR)/load.tcl

set filename [file join $::env(WORK_HOME) "macro-placement.tcl"]
puts "Macro placement written to $filename"

# good 'nuf for pin placement for now
load_design 2_floorplan.odb 2_floorplan.sdc

write_macro_placement $filename.tmp
set f [open $filename.tmp r]
set content [read $f]
# Uncomment when using this in a SYNTH_HIERARCHICAL=0 context
#
# set content [string map {"/" "."} $content]
close $f

set f [open $filename w]
puts -nonewline $f $content
close $f
