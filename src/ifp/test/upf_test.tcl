source "helpers.tcl"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_top.v
link_design mpd_top

read_upf -file upf/mpd_top.upf


set_domain_area PD_D1 -area {27 27 60 60}
set_domain_area PD_D2 -area {100 100 180 180}
set_domain_area PD_D3 -area {200 200 300 300}
set_domain_area PD_D4 -area {300 300 400 400}


set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]


puts "Power Domains List:"
set pds [$block getPowerDomains]
foreach pd $pds {
    lassign [$pd getArea] present area
    if { $present } {
        set area [list [$area xMin] [$area yMin] [$area xMax] [$area yMax]]
    } else {
        set area "unset"
    }
    puts "PowerDomain: [$pd getName], Elements: [$pd getElements], Area: $area"
}


set insts [$block getInsts]
foreach inst $insts {
    puts "Instance: [$inst getName] - "
    set iterms [$inst getITerms]
    foreach iterm $iterms {
        set net [$iterm getNet]
        set io_type [$iterm getIoType]
        if { $net != "NULL" && $io_type == "OUTPUT" } {
            set connected_iterms [$net getITerms]
            foreach conn $connected_iterms {
                set connected_inst [$conn getInst]
                if {[$inst getName] != [$connected_inst getName]} {
                    puts " -> [$net getName] -> [$connected_inst getName]"
                }
            }
        }

    }
}


initialize_floorplan -die_area { 0 0 500 500 } \
    -core_area { 100 100 400 400 } \
    -site unithd


set insts [$block getInsts]
foreach inst $insts {
    set group [$inst getGroup]
    set group_name ""
    if { $group != "NULL" } {
        set group_name [$group getName] 
    }
    set dbMaster [$inst getMaster]
    puts "Instance: [$inst getName] - ($group_name) : [$dbMaster getName] "
    set iterms [$inst getITerms]
    foreach iterm $iterms {
        set net [$iterm getNet]
        set io_type [$iterm getIoType]
        if { $net != "NULL" && $io_type == "OUTPUT" } {
            set connected_iterms [$net getITerms]
            foreach conn $connected_iterms {
                set connected_inst [$conn getInst]
                # puts " -> [$connected_inst getName] through [$net getName]"
                if {[$inst getName] != [$connected_inst getName]} {
                    puts " -> [$net getName] -> [$connected_inst getName] "
                }
            }
        }

    }
}
