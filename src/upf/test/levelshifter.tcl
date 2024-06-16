source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog data/mpd_top/mpd_top.v
link_design mpd_top

read_upf -file data/mpd_top/mpd_top_ls.upf


set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set pd [$block findPowerDomain PD_AES_1]
puts "PowerDomain: [$pd getName]"

set levelshifters [$pd getLevelShifters]

foreach ls $levelshifters {
    puts "LevelShifter: [$ls getName], Source: [$ls getSource]"
    puts "Sink: [$ls getSink], Functional: [$ls isUseFunctionalEquivalence], AppliesTo: [$ls getAppliesTo], AppliesToBoundary: [$ls getAppliesToBoundary]"
    puts "Rule: [$ls getRule], Threshold: [$ls getThreshold], NoShift: [$ls isNoShift], ForceShift: [$ls isForceShift], Location: [$ls getLocation]"
    puts "InputSupply: [$ls getInputSupply], OutputSupply: [$ls getOutputSupply], InternalSupply: [$ls getInternalSupply]"
    puts "NamePrefix: [$ls getNamePrefix], NameSuffix: [$ls getNameSuffix]"

    set els [$ls getElements]
    foreach el $els {
      puts "Included Element: $el"
    }

    set excluded [$ls getExcludeElements]
    foreach ex $excluded {
      puts "Excluded Element: $ex"
    }

}




