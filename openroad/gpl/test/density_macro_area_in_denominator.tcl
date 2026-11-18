source helpers.tcl
read_lef ./nangate45.lef
read_lef ./bp_be_top_macro.lef
read_def ./macro01.def

set density [gpl::get_global_placement_uniform_density -macro_area_in_denominator]
puts "density -> [format {%.3f} $density]"
