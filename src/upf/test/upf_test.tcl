source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog data/mpd_top/mpd_top.v
link_design mpd_top

read_upf -file data/mpd_top/mpd_top.upf


set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]


puts "Power Domains List:"
set pds [$block getPowerDomains]
foreach pd $pds {
    puts "PowerDomain: [$pd getName], Elements: [$pd getElements]"   
}

puts "Power Domain Info:"
set pds {PD_AES_1 PD_AES_2}
foreach pd $pds {
    set pd_inst [$block findPowerDomain $pd]
    set pswitches [$pd_inst getPowerSwitches]
    set isolations [$pd_inst getIsolations]  
    puts "PowerDomain: $pd"

    foreach switch $pswitches {
        puts "PowerSwitch: [$switch getName], OutSupply: [$switch getOutputSupplyPorts], InSupply: [$switch getInputSupplyPorts], ControlPorts: {[$switch getControlPorts]}, On States {[$switch getOnStates]}" 
        set lib_cell [$switch getLibCell]
        puts "Lib Cell: [$lib_cell getName]"
    }

    foreach iso $isolations {
        puts "Isolation: [$iso getName], AppliesTo: [$iso getAppliesTo], Clamp: [$iso getClampValue], Signal: [$iso getIsolationSignal], Sense: [$iso getIsolationSense], Location: [$iso getLocation]"
    }

}

puts "Logic Ports List:"
set lps [$block getLogicPorts]
foreach lp $lps {
    puts "LogicPort: [$lp getName], Direction: [$lp getDirection]"
}





