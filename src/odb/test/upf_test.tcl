source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
# read_verilog data/mpd_top/mpd_top.v
read_verilog data/mpd_top/1_synth.v
link_design mpd_top

read_upf -file data/mpd_top/mpd_top.upf


set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]

set top_module [$block getTopModule]
puts "Top Module Name: [$top_module getName]"
puts "Top Module HName: [$top_module getHierarchicalName]"
# set top_module_inst [$top_module getModInst]
# puts "Top Module Inst Name: [$top_module_inst getName]"
# puts "Top Module Inst HName: [$top_module_inst getHierarchicalName]"
set top_mod_instances [$top_module getInsts]

# puts "Top Modules Instances: "
# foreach inst $top_mod_instances {
#     puts "Instance: [$inst getName]"
# }

set children [$top_module getChildren]
foreach mod_inst $children {
    puts "Child Mod Insts: [$mod_inst getHierarchicalName]"
    set child_mod [$mod_inst getMaster]
    puts "Child Mod: [$child_mod getName]"
    set child_mod_inst [$child_mod getModInst]
    puts "Back to Child Inst from child Mod: [$child_mod_inst getHierarchicalName]"

    # set grand_children [$child_mod getChildren]
    # foreach g_child $grand_children {      
    #     puts "Grandchild Mod Insts: [$g_child getHierarchicalName]"
    # }
}


puts "Power Domains List:"
set pds [$block getPowerDomains]
foreach pd $pds {
    puts "PowerDomain: [$pd getName], Elements: [$pd getElements]"   
}

# puts "Power Domain Info:"
# set pds {PD_AES_1 PD_AES_2}
# foreach pd $pds {
#     set pd_inst [$block findPowerDomain $pd]
#     set pswitches [$pd_inst getPowerSwitches]
#     set isolations [$pd_inst getIsolations]  
#     puts "PowerDomain: $pd"

#     foreach switch $pswitches {
#         puts "PowerSwitch: [$switch getName], OutSupply: [$switch getOutSupplyPort], InSupply: [$switch getInSupplyPort], ControlPorts: {[$switch getControlPorts]}, On States {[$switch getOnStates]}"  
#     }

#     foreach iso $isolations {
#         puts "Isolation: [$iso getName], AppliesTo: [$iso getAppliesTo], Clamp: [$iso getClampValue], Signal: [$iso getIsolationSignal], Sense: [$iso getIsolationSense], Location: [$iso getLocation]"
#     }

# }

# puts "Logic Ports List:"
# set lps [$block getLogicPorts]
# foreach lp $lps {
#     puts "LogicPort: [$lp getName], Direction: [$lp getDirection]"
# }





