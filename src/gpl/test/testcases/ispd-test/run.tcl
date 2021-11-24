# 
# Examples for Non Timing-driven RePlAce with TCL usage
#

set design ispd18_test1.input 

replace_external rep

# Import LEF/DEF files
rep import_lef ispd18_test1.input.lef
rep import_def ispd18_test1.input.def

rep set_verbose_level 0

# Initialize RePlAce
rep init_replace

# place_cell with BiCGSTAB 
rep place_cell_init_place


# print out instances' x/y coordinates
#rep print_instances

# place_cell with Nesterov method
rep place_cell_nesterov_place

# print out instances' x/y coordinates
#rep print_instances

# Export DEF file
rep export_def ./${design}_nontd.def
puts "Final HPWL: [rep get_hpwl]"
