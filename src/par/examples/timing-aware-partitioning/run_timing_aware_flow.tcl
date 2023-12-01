######################################################################################
### Example scripts for running timing-aware partitioning
### DATE: 2023-05-22
######################################################################################

######################################################################################
### Read the solution file from triton_part_design
### and convert it to hMETIS format
proc read_design_part_solution_file { filename filename_update } {
  set file [open $filename r]
  set solution_vec [list]
  while {[gets $file line] >= 0} {
    set items [split $line " "]
    lappend solution_vec [lindex $items 2]        
  }
  close $file

  set updated_file $filename_update
  set file [open ${updated_file} w]
  foreach value $solution_vec {
    puts $file $value
  }
  close $file
}
########################################################################################


########################################################################################
### Set global variables
########################################################################################
set lib_setup_file lib_setup.tcl
set design_setup_file design_setup.tcl

source $lib_setup_file
source $design_setup_file

foreach lef_file ${lefs} {
  read_lef $lef_file
}

foreach lib_file ${libs} {
  read_liberty $lib_file
}

read_verilog $netlist
link_design $top_design
read_sdc $sdc

# check the timing driven flag
set num_parts 5
set balance_constraint 2
set seed 0
set top_n 100000
set evaluate_top_n 100000
# set the extra_delay_cut to 20% of the clock period
# the extra_delay_cut is introduced for each cut hyperedge
set extra_delay_cut 9.2e-10  
set timing_aware_flag true
set timing_guardband true
set part_design_solution_file "${design}_part_design.hgr.part.${num_parts}"
set part_design_solution_file_update ${part_design_solution_file}.update


##############################################################################################
### TritonPart with slack progagation
##############################################################################################
puts "Start TritonPart with slack propagation"
# call triton_part to partition the netlist
triton_part_design -num_parts $num_parts -balance_constraint $balance_constraint \
                   -seed $seed -top_n $top_n \
                   -timing_aware_flag $timing_aware_flag -extra_delay $extra_delay_cut \
                   -guardband_flag $timing_guardband \
                   -solution_file $part_design_solution_file 

puts "Convert the solution format for transformation"
# convert the solution into the format as hmetis
read_design_part_solution_file $part_design_solution_file $part_design_solution_file_update

puts "Evaluate the solution"
# evaluate the solution
# we should turn off the guardband_flag
evaluate_part_design_solution -num_parts $num_parts -balance_constraint $balance_constraint \
                              -timing_aware_flag $timing_aware_flag -extra_delay $extra_delay_cut \
                              -top_n $evaluate_top_n -guardband_flag false \
                              -solution_file ${part_design_solution_file_update} 


puts "Finish TritonPart with slack propagation"
exit
