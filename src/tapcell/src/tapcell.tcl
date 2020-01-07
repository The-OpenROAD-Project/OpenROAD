################################################################################
## Authors: Eder Matheus Monteiro (UFRGS) and Minsoo Kim (UCSD)
##
## BSD 3-Clause License
##
## Copyright (c) 2019, Federal University of Rio Grande do Sul (UFRGS)
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
################################################################################

sta::define_cmd_args "tapcell" {[-tapcell_master tapcell_master] \
                                    [-endcap_master endcap_master] \
                                    [-endcap_cpp endcap_cpp] \
#when you set 25 (um), each row has 50um with checker-board pattern
                                    [-distance dist] \
                                    [-halo_width_x halo_x] \
                                    [-halo_width_y halo_y] \
#the next parameters are for 14nm tech
                                    [-tap_nwin2_master tap_nwin2_master] \
                                    [-tap_nwin3_master tap_nwin3_master] \
                                    [-tap_nwout2_master tap_nwout2_master] \
                                    [-tap_nwout3_master tap_nwout3_master] \
                                    [-tap_nwintie_master tap_nwintie_master] \
                                    [-tap_nwouttie_master tap_nwouttie_master] \
                                    [-cnrcap_nwin_master cnrcap_nwin_master] \
                                    [-cnrcap_nwout_master cnrcap_nwout_master] \
                                    [-incnrcap_nwin_master incnrcap_nwin_master] \
                                    [-incnrcap_nwout_master incnrcap_nwout_master] \
                                    [-tbtie_cpp tbtie_cpp] \
#the next flags enables gf14 flow
                                    [-no_cell_at_top_bottom] \
                                    [-add_boundary_cell] \
}

#Pre-step: assumed that placement blockages inserted around macros
#You might or might not need this step

namespace eval tapcell {
    #proc to detect even/odd
    proc even {row} {
        set db [::ord::get_db]
        set block [[$db getChip] getBlock]

        set site_y [[$row getSite] getHeight]

        set lly [[$row getBBox] yMin]
        set core_box_lly [[$block getBBox] yMin]
        set lly_idx [expr ($lly-$core_box_lly)/$site_y]
        set lly_idx_int [expr int($lly_idx)]

        if {[expr $lly_idx_int % 2]==0} {
            return 1
        } else {
            return 0
        }
    }

    proc get_min_rows_y {rows} {
        set min_y -1
        foreach row $rows {
            set row_y [[$row getBBox] yMin]
            if {$min_y == -1} {
                set min_y $row_y
            }

            if {$row_y < $min_y} {
                set min_y $row_y
            }
        }

        return $min_y
    }

    proc get_max_rows_y {rows} {
        set max_y -1
        foreach row $rows {
            set row_y [[$row getBBox] yMax]
            if {$row_y > $max_y} {
                set max_y $row_y
            }
        }

        return $max_y
    }

    proc get_min_rows_x {rows} {
        set min_x -1
        foreach row $rows {
            set row_x [[$row getBBox] xMin]
            if {$min_x == -1} {
                set min_x $row_x
            }

            if {$row_x < $min_x} {
                set min_x $row_x
            }
        }

        return $min_x
    }

    proc get_max_rows_x {rows} {
        set max_x -1
        foreach row $rows {
            set row_x [[$row getBBox] xMax]
            if {$row_x > $max_x} {
                set max_x $row_x
            }
        }

        return $max_x
    }

    #proc to detect top/bottom row
    proc top_or_bottom {row min_y max_y} {
        set db [::ord::get_db]
        set block [[$db getChip] getBlock]

        set lly [[$row getBBox] yMin]
        set ury [[$row getBBox] yMax]
        set core_box_lly [[$block getBBox] yMin]
        set core_box_ury [[$block getBBox] yMax]
        
        if {$lly == $min_y} {
            return -1
        } elseif {$ury == $max_y} {
            return 1
        } else {
            return 0
        }
    }

    #proc to detect if row is right below/above macro block
    proc right_above_below_macros {blockages row halo_x halo_y} {
        set row_llx [[$row getBBox] xMin]
        set row_lly [[$row getBBox] yMin]
        set row_urx [[$row getBBox] xMax]
        set row_ury [[$row getBBox] yMax]
        
        set row_height [expr $row_ury - $row_lly]

        set row_below_ury [expr $row_ury + $row_height]
        set row_above_lly [expr $row_lly - $row_height]

        foreach blockage $blockages {
            set blockage_llx [expr [[$blockage getBBox] xMin] - $halo_x]
            set blockage_lly [expr [[$blockage getBBox] yMin] - $halo_y]
            set blockage_urx [expr [[$blockage getBBox] xMax] + $halo_x]
            set blockage_ury [expr [[$blockage getBBox] yMax] + $halo_y]

            set min_x [expr max($blockage_llx, $row_llx)]
            set max_x [expr min($blockage_urx, $row_urx)]

            set min_above_y [expr max($blockage_lly, $row_above_lly)]
            set max_below_y [expr min($blockage_ury, $row_below_ury)]
            
            set min_y [expr max($blockage_lly, $row_lly)]
            set max_y [expr min($blockage_ury, $row_ury)]

            set dx [expr $min_x - $max_x]
            
            set dy_above [expr $min_above_y - $max_y]
            set dy_below [expr $min_y - $max_below_y]

            if {[expr ($dx < 0) && ($dy_above < 0)]} {
                return 1
            }

            if {[expr ($dx < 0) && ($dy_below < 0)]} {
                return 1
            }
        }
        return 0
    }

    #proc to detect if blockage overlaps with row
    proc overlaps {blockage row halo_x halo_y} {
        set blockage_llx [expr [[$blockage getBBox] xMin] - $halo_x]
        set blockage_lly [expr [[$blockage getBBox] yMin] - $halo_y]
        set blockage_urx [expr [[$blockage getBBox] xMax] + $halo_x]
        set blockage_ury [expr [[$blockage getBBox] yMax] + $halo_y]

        set row_llx [[$row getBBox] xMin]
        set row_lly [[$row getBBox] yMin]
        set row_urx [[$row getBBox] xMax]
        set row_ury [[$row getBBox] yMax]

        set min_x [expr max($blockage_llx, $row_llx)]
        set max_x [expr min($blockage_urx, $row_urx)]
        set min_y [expr max($blockage_lly, $row_lly)]
        set max_y [expr min($blockage_ury, $row_ury)]

        set dx [expr $min_x - $max_x]
        set dy [expr $min_y - $max_y]

        set overlap [expr ($dx < 0) && ($dy < 0)]

        return $overlap
    }

    proc get_insts_in_area {area block} {
        set ll_x [lindex $area 0]
        set ll_y [lindex $area 1]
        set ur_x [lindex $area 2]
        set ur_y [lindex $area 3]

        set insts_in_area ""

        foreach inst [$block getInsts] {
            set inst_llx [[$inst getBBox] xMin]
            set inst_lly [[$inst getBBox] yMin]
            set inst_urx [[$inst getBBox] xMax]
            set inst_ury [[$inst getBBox] yMax]

            if {($inst_llx >= $ll_x) && ($inst_lly >= $ll_y) && \
                ($inst_urx <= $ur_x) && ($inst_ury <= $ur_y)} {
                lappend insts_in_area $inst
            }
        }

        return $insts_in_area
    }

    proc find_endcaps {y lib block} {
        set sites [$lib getSites]
        set site_y [[lindex $sites 0] getHeight]
        
        set rows [$block getRows]

        set min_x [tapcell::get_min_rows_x $rows]
        set max_x [tapcell::get_max_rows_x $rows]

        set llx [expr $min_x + 0.001]
        set lly [expr $y + 0.001]
        set urx [expr $max_x - 0.001]
        set ury [expr $y + $site_y - 0.001]
        set area "$llx $lly $urx $ury"

        set insts [get_insts_in_area $area $block]

        set endcaps ""
        foreach inst $insts {
            set cell_name [[$inst getMaster] getName]
            if {[string match "ENDCAPTIE*" $cell_name]} {
                lappend endcaps $inst
            }
        }

        return $endcaps
    }

    proc find_endcaps_above {y lib block} {
        set sites [$lib getSites]
        set site_y [[lindex $sites 0] getHeight]

        set rows [$block getRows]

        set llx [tapcell::get_min_rows_x $rows]
        set urx [tapcell::get_max_rows_x $rows]
        set ury [expr $y + $site_y]

        #search above the row
        set up_llx [expr $llx + 0.001]
        set up_lly [expr $y + $site_y + 0.001]
        set up_urx [expr $urx - 0.001]
        set up_ury [expr $ury + $site_y - 0.001]
        set up_area "$up_llx $up_lly $up_urx $up_ury"

        set insts [get_insts_in_area $up_area $block]

        set endcaps ""

        foreach inst $insts {
            set cell_name [[$inst getMaster] getName]
            if {[string match "ENDCAPTIE*" $cell_name]} {
                lappend endcaps $inst
            }
        }

        return $endcaps
    }
}

# Main function. It will run tapcell given the correct parameters
proc tapcell { args } {
    sta::parse_key_args "tapcell" args \
        keys {-tapcell_master -endcap_master -endcap_cpp -distance -halo_width_x \
              -halo_width_y -tap_nwin2_master -tap_nwin3_master -tap_nwout2_master \
              -tap_nwout3_master -tap_nwintie_master -tap_nwouttie_master \
              -cnrcap_nwin_master -cnrcap_nwout_master -incnrcap_nwin_master \
              -incnrcap_nwout_master -tbtie_cpp} \
              flags {-no_cell_at_top_bottom -add_boundary_cell}

    if { [info exists keys(-tapcell_master)] } {
        set tapcell_master $keys(-tapcell_master)
    }

    if { [info exists keys(-endcap_master)] } {
        set endcap_master $keys(-endcap_master)
    }

    if { [info exists keys(-endcap_cpp)] } {
        set endcap_cpp $keys(-endcap_cpp)
    }

    if { [info exists keys(-distance)] } {
        set dist $keys(-distance)
    } else {
        set dist 2
    }

    if { [info exists keys(-halo_width_y)] } {
        set halo_y $keys(-halo_width_y)
    } else {
        set halo_y 2
    }

    if { [info exists keys(-halo_width_x)] } {
        set halo_x $keys(-halo_width_x)
    } else {
        set halo_x 2
    }

    if { [info exists keys(-tap_nwin2_master)] } {
        set tap_nwin2_master $keys(-tap_nwin2_master)
    }

    if { [info exists keys(-tap_nwin3_master)] } {
        set tap_nwin3_master $keys(-tap_nwin3_master)
    }

    if { [info exists keys(-tap_nwout2_master)] } {
        set tap_nwout2_master $keys(-tap_nwout2_master)
    }

    if { [info exists keys(-tap_nwout3_master)] } {
        set tap_nwout3_master $keys(-tap_nwout3_master)
    }

    if { [info exists keys(-tap_nwintie_master)] } {
        set tap_nwintie_master $keys(-tap_nwintie_master)
    }

    if { [info exists keys(-tap_nwouttie_master)] } {
        set tap_nwouttie_master $keys(-tap_nwouttie_master)
    }

    if { [info exists keys(-cnrcap_nwin_master)] } {
        set cnrcap_nwin_master $keys(-cnrcap_nwin_master)
    }

    if { [info exists keys(-cnrcap_nwout_master)] } {
        set cnrcap_nwout_master $keys(-cnrcap_nwout_master)
    }

    if { [info exists keys(-incnrcap_nwin_master)] } {
        set incnrcap_nwin_master $keys(-incnrcap_nwin_master)
    }

    if { [info exists keys(-incnrcap_nwout_master)] } {
        set incnrcap_nwout_master $keys(-incnrcap_nwout_master)
    }

    if { [info exists keys(-tbtie_cpp)] } {
        set tbtie_cpp $keys(-tbtie_cpp)
    }

    if { [info exists flags(-add_boundary_cell)] } {
        set add_boundary_cell true
    } else {
        set add_boundary_cell false
    }

    if { [info exists flags(-no_cell_at_top_bottom)] } {
        set no_cell_at_top_bottom true
    } else {
        set no_cell_at_top_bottom false
    }

    puts "Running tapcell..."
        
    set db [::ord::get_db]
    set block [[$db getChip] getBlock]
    set libs [$db getLibs]

    foreach lib $libs {
        set lef_units [$lib getLefUnits]
    }

    set lib [lindex $libs 0]

    set halo_y [expr $halo_y * $lef_units]
    set halo_x [expr $halo_x * $lef_units]

    #Step 1: cut placement rows if there are overlaps between rows and placement blockages
    puts "Step1: Cut rows..."

    set block_count 0
    set cut_rows_count 0

    set blockages {}

    foreach blockage [$block getInsts] {
        set inst_master [$blockage getMaster]
        if { [string match [$inst_master getType] "BLOCK"] } {
            lappend blockages $blockage
        }
    }

    set rows_count 0
    foreach row [$block getRows] {
        incr rows_count
    }

    foreach blockage $blockages {
        set rows [$block getRows]
        incr block_count
        foreach row $rows {
            if {[tapcell::overlaps $blockage $row $halo_x $halo_y]} {
                incr cut_rows_count
                # Create two new rows, avoiding overlap with blockage
                set row_site [$row getSite]
                set orient [$row getOrient]
                set direction [$row getDirection]

                set site_width [$row_site getWidth]

                set row1_name [$row getName]
                append row1_name "_1"

                set row2_name [$row getName]
                append row2_name "_2"

                ## First new row: from left of original row to the left boundary of blockage
                set row1_origin_x [[$row getBBox] xMin]
                set row1_origin_y [[$row getBBox] yMin]
                set row1_end_x [expr [[$blockage getBBox] xMin] - $halo_x]
                set row1_num_sites [expr {($row1_end_x - $row1_origin_x)/$site_width}]

                if {$row1_num_sites > 0} {
                    odb::dbRow_create $block $row1_name $row_site $row1_origin_x $row1_origin_y $orient $direction $row1_num_sites $site_width
                }

                ## Second new row: from right of original  row to the right boundary of blockage
                set blockage_x_max [[$blockage getBBox] xMax]

                set row2_origin_x_tmp [expr {ceil (1.0*($blockage_x_max + $halo_x)/$site_width)*$site_width}]
                set row2_origin_x [expr { int($row2_origin_x_tmp) }]
                set row2_origin_y [[$row getBBox] yMin]
                set row2_end_x [[$row getBBox] xMax]
                set row2_num_sites [expr {($row2_end_x - $row2_origin_x)/$site_width}]

                if {$row2_num_sites > 0} {
                    odb::dbRow_create $block $row2_name $row_site $row2_origin_x $row2_origin_y $orient $direction $row2_num_sites $site_width
                }

                # Remove current row
                odb::dbRow_destroy $row
            }
        }
    }

    puts "---- Macro blocks found: $block_count"
    puts "---- #Original rows: $rows_count"
    puts "---- #Cut rows: $cut_rows_count"

    #Step 2: Insert Endcap at the left and right end of each row
    puts "Step2: Insert endcaps..."
    
    set rows [$block getRows]
    
    set min_y [tapcell::get_min_rows_y $rows]
    set max_y [tapcell::get_max_rows_y $rows]

    set cnt 0
    set endcap_count 0
    foreach row $rows {
        set master [$db findMaster $endcap_master]

        if { ![string match [$master getConstName] $endcap_master] } {
            puts "ERROR: Master $endcap_master not found"
        }

        set ori [$row getOrient]

        set top_bottom [tapcell::top_or_bottom $row $min_y $max_y]

        if {$no_cell_at_top_bottom == true} {
            if {$top_bottom == 1} {
                if {[string match "MX" $ori]} {
                    set master [$db findMaster $cnrcap_nwin_master]

                    if { ![string match [$master getConstName] $cnrcap_nwin_master] } {
                        puts "ERROR: Master $cnrcap_nwin_master not found"
                        exit 1
                    }
                } else {
                    set master [$db findMaster $cnrcap_nwout_master]
                    
                    if { ![string match [$master getConstName] $cnrcap_nwout_master] } {
                        puts "ERROR: Master $cnrcap_nwout_master not found"
                        exit 1
                    }
                }
            } elseif {$top_bottom == -1} {
                if {[string match "R0" $ori]} {
                    set master [$db findMaster $cnrcap_nwin_master]

                    if { ![string match [$master getConstName] $cnrcap_nwin_master] } {
                        puts "ERROR: Master $cnrcap_nwin_master not found"
                        exit 1
                    }
                } else {
                    set master [$db findMaster $cnrcap_nwout_master]

                    if { ![string match [$master getConstName] $cnrcap_nwout_master] } {
                        puts "ERROR: Master $cnrcap_nwout_master not found"
                        exit 1
                    }
                }
            } else {
                set master [$db findMaster $endcap_master]
            }
        }

        set master_x [$master getWidth]
        set master_y [$master getHeight]

        set llx [[$row getBBox] xMin]
        set lly [[$row getBBox] yMin]
        set urx [[$row getBBox] xMax]
        set ury [[$row getBBox] yMax]

        set loc_2_x [expr $urx - $master_x]
        set loc_2_y [expr $ury - $master_y]

        set inst1_name "PHY_${cnt}"
        set inst1 [odb::dbInst_create $block $master $inst1_name]
        $inst1 setOrient $ori
        $inst1 setLocation $llx $lly
        $inst1 setPlacementStatus LOCKED

        incr cnt
        incr endcap_count

        set inst2_name "PHY_${cnt}"
        set inst2 [odb::dbInst_create $block $master $inst2_name]
        $inst2 setOrient $ori
        $inst2 setLocation $loc_2_x $loc_2_y
        $inst2 setPlacementStatus LOCKED

        incr cnt
        incr endcap_count
    }

    puts "---- #Endcaps inserted: $endcap_count"

    #Step 3: Insert tap
    puts "Step3: Insert tapcells..."

    set min_x [tapcell::get_min_rows_x $rows]
    set max_x [tapcell::get_max_rows_x $rows]

    set min_y [tapcell::get_min_rows_y $rows]
    set max_y [tapcell::get_max_rows_y $rows]
    
    set tapcell_count 0
    foreach row $rows {
        set site_x [[$row getSite] getWidth]
        set llx [[$row getBBox] xMin]
        set lly [[$row getBBox] yMin]
        set urx [[$row getBBox] xMax]
        set ury [[$row getBBox] yMax]

        set ori [$row getOrient]

        if {[tapcell::even $row]} {
            set offset [expr $dist*$lef_units]
        } else {
            set offset [expr $dist*2*$lef_units]
        }

        if {[tapcell::top_or_bottom $row $min_y $max_y]} {
            if {$no_cell_at_top_bottom == true} {
                continue
            }
            set pitch [expr $dist*$lef_units]
            set offset [expr $dist*$lef_units]
        } elseif {[tapcell::right_above_below_macros $blockages $row $halo_x $halo_y]} {
            set pitch [expr $dist*$lef_units]
            set offset [expr $dist*$lef_units]
        } else {
            set pitch [expr $dist*2*$lef_units]
        }

        for {set x [expr $llx+$offset]} {$x < [expr $urx-$endcap_cpp*$site_x]} {set x [expr $x+$pitch]} {
            set master [$db findMaster $tapcell_master]
            set inst_name "PHY_${cnt}"

            if { [string match [$master getConstName] $tapcell_master] } {
                set x_tmp [expr {ceil (1.0*$x/$site_x)*$site_x}]
                set x [expr { int($x_tmp) }]
                set x_end [expr $x + $site_x]

                if {($x != $min_x) && ($x_end != $max_x)} {
                    set inst [odb::dbInst_create $block $master $inst_name]
                    $inst setOrient $ori

                    $inst setLocation $x $lly
                    $inst setPlacementStatus LOCKED

                    incr cnt
                    incr tapcell_count
                }
            }
        }
    }

    puts "---- #Tapcells inserted: $tapcell_count"

    if {$add_boundary_cell == true} {
        #Step 4: Insert top/bottom
        #Step 4-1: insert top/bottom between cnr cell
        puts "Step4.1: Insert tapcells at top/bottom between cnr cell..."

        set topbottom_cnt 0

        foreach row $rows {
            set site_x [[$row getSite] getWidth]

            set llx [[$row getBBox] xMin]
            set lly [[$row getBBox] yMin]
            set urx [[$row getBBox] xMax]
            set ury [[$row getBBox] yMax]

            set ori [$row getOrient]

            set tbtiewidth [expr $tbtie_cpp*$site_x]
            set endcapwidth [expr $endcap_cpp*$site_x]

            set topbottom_chk [tapcell::top_or_bottom $row $min_y $max_y]
            
            set master NULL
            set tb2_master NULL
            set tb3_master NULL

            if {$topbottom_chk == 1} {
                if {[string match "MX" $ori]} {
                    set master [$db findMaster $tap_nwintie_master]
                    set tb2_master [$db findMaster $tap_nwin2_master]
                    set tb3_master [$db findMaster $tap_nwin3_master]
                } else {
                    set master [$db findMaster $tap_nwouttie_master]
                    set tb2_master [$db findMaster $tap_nwout2_master]
                    set tb3_master [$db findMaster $tap_nwout3_master]
                }
            } elseif {$topbottom_chk == -1} {
                if {[string match "R0" $ori]} {
                    set master [$db findMaster $tap_nwintie_master]
                    set tb2_master [$db findMaster $tap_nwin2_master]
                    set tb3_master [$db findMaster $tap_nwin3_master]
                } else {
                    set master [$db findMaster $tap_nwouttie_master]
                    set tb2_master [$db findMaster $tap_nwout2_master]
                    set tb3_master [$db findMaster $tap_nwout3_master]
                }
            }

            #insert tb tie
            if {$topbottom_chk != 0} {
                set x_start [expr $llx+$endcapwidth]
                set x_end [expr $urx-$endcapwidth]
                for {set x $x_start} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                    set inst_name "PHY_${cnt}"
                    set new_inst [odb::dbInst_create $block $master $inst_name]
                    $new_inst setOrient $ori
                    $new_inst setLocation $x $lly
                    $new_inst setPlacementStatus LOCKED

                    incr cnt
                    incr topbottom_cnt
                }

                set numcpp [format %i [expr int(($x_end - $x)/$site_x)]]

                if {[expr $numcpp % 2] == 1} {
                    set inst_name "PHY_${cnt}"
                    set x_tb3 [expr $x_end-(3*$site_x)]
                    set new_inst [odb::dbInst_create $block $tb3_master $inst_name]
                    $new_inst setOrient $ori
                    $new_inst setLocation $x_tb3 $lly
                    $new_inst setPlacementStatus LOCKED

                    incr cnt
                    incr topbottom_cnt
                    set x_end $x_tb3
                }

                for {} {$x < $x_end} {set x [expr $x+(2*$site_x)]} {
                    set inst_name "PHY_${cnt}"
                    set new_inst [odb::dbInst_create $block $tb2_master $inst_name]
                    $new_inst setOrient $ori
                    $new_inst setLocation $x $lly
                    $new_inst setPlacementStatus LOCKED

                    incr cnt
                    incr topbottom_cnt
                }
            }
        }

        puts "Top/bottom cells inserted: $topbottom_cnt"

        #Step 4-2: insert incnr/topbottom for blkgs
        
        set blkgs_cnt 0
        
        puts "Step4.2: Insert tapcells incnr/top/bottom for blkgs..."
        set corebox_llx [tapcell::get_min_rows_x $rows]
        set corebox_urx [tapcell::get_max_rows_x $rows]

        set corebox_lly [tapcell::get_min_rows_y $rows]
        set corebox_ury [tapcell::get_max_rows_y $rows]

        set sites [$lib getSites]
        set site_x [[lindex $sites 0] getWidth]
        set site_y [[lindex $sites 0] getHeight]
        
        for {set y [expr $corebox_lly+$site_y]} {$y < [expr $corebox_ury-$site_y]} {set y [expr $y + $site_y]} {
            foreach temp_row $rows {
                set temp_row_lly [[$temp_row getBBox] yMin]
                if {($temp_row_lly == $y)} {
                    set row $temp_row
                    break
                }
            }

            set ori [$row getOrient]

            set endcaps [tapcell::find_endcaps $y $lib $block]
            set endcaps_above [tapcell::find_endcaps_above $y $lib $block]

            set endcap_xs ""
            set endcap_above_xs ""

            foreach endcap $endcaps {
                lappend endcap_xs [[$endcap getBBox] xMin]
            }

            foreach endcap $endcaps_above {
                lappend endcap_above_xs [[$endcap getBBox] xMin]
            }

            set endcap_xs [lsort -increasing $endcap_xs]
            set endcap_above_xs [lsort -increasing $endcap_above_xs]

            if {$endcap_xs != $endcap_above_xs} {
                set endcapnum [llength $endcap_xs]
                set endcapnum_above [llength $endcap_above_xs]

                #top
                if {$endcapnum < $endcapnum_above} {
                    if {[string match "MX" $ori]} {
                        set incnr_master [$db findMaster $incnrcap_nwin_master]
                        set tb2_master [$db findMaster $tap_nwin2_master]
                        set tb3_master [$db findMaster $tap_nwin3_master]
                        set tbtie_master [$db findMaster $tap_nwintie_master]
                    } else {
                        set incnr_master [$db findMaster $incnrcap_nwout_master]
                        set tb2_master [$db findMaster $tap_nwout2_master]
                        set tb3_master [$db findMaster $tap_nwout3_master]
                        set tbtie_master [$db findMaster $tap_nwouttie_master]
                    }

                    set res {}
                    foreach item $endcap_above_xs {
                        if {$item ni $endcap_xs} {
                            lappend res $item
                        }
                    }

                    #puts $res

                    for {set i 0} {$i < [llength $res]} {set i [expr $i+2]} {
                        set x_start [lindex $res $i]
                        set x_end [lindex $res $i+1]

                        #insert incnr
                        set inst1_name "PHY_${cnt}"
                        set inst1 [odb::dbInst_create $block $incnr_master $inst1_name]
                        $inst1 setOrient $ori
                        $inst1 setLocation $x_start $y
                        $inst1 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt

                        set inst2_name "PHY_${cnt}"
                        set inst2 [odb::dbInst_create $block $incnr_master $inst2_name]
                        $inst2 setOrient $ori
                        $inst2 setLocation $x_end $y
                        $inst2 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt

                        for {set x [expr $x_start+$endcapwidth]} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                            set inst3_name "PHY_${cnt}"
                            set inst3 [odb::dbInst_create $block $tbtie_master $inst3_name]
                            $inst3 setOrient $ori
                            $inst3 setLocation $x $y
                            $inst3 setPlacementStatus LOCKED

                            incr cnt
                            incr blkgs_cnt
                        }

                        set numcpp [format %i [expr int(($x_end - $x)/$site_x)]]

                        if {[expr $numcpp % 2] == 1} {
                            set x_tb3 [expr $x_end-(3*$site_x)]
                            set inst4_name "PHY_${cnt}"
                            set inst4 [odb::dbInst_create $block $tb3_master $inst4_name]
                            $inst4 setOrient $ori
                            $inst4 setLocation $x_tb3 $y
                            $inst4 setPlacementStatus LOCKED

                            incr cnt
                            incr blkgs_cnt
                            set x_end $x_tb3
                        }

                        for {} {$x < $x_end} {set x [expr $x+(2*$site_x)]} {
                            set inst5_name "PHY_${cnt}"
                            set inst5 [odb::dbInst_create $block $tb2_master $inst5_name]
                            $inst5 setOrient $ori
                            $inst5 setLocation $x $y
                            $inst5 setPlacementStatus LOCKED

                            addInst -cell $tb2_master -inst "PHY_${cnt}" -physical -loc $loc -ori $ori
                            incr cnt
                            incr blkgs_cnt
                        }
                    }
                } else {
                    #targeting upper row
                    if {[string match "R0" $ori]} {
                        set ori "MX"
                    } else {
                        set ori "R0"
                    }

                    if {[string match "R0" $ori]} {
                        set incnr_master [$db findMaster $incnrcap_nwin_master]
                        set tb2_master [$db findMaster $tap_nwin2_master]
                        set tb3_master [$db findMaster $tap_nwin3_master]
                        set tbtie_master [$db findMaster $tap_nwintie_master]
                    } else {
                        set incnr_master [$db findMaster $incnrcap_nwout_master]
                        set tb2_master [$db findMaster $tap_nwout2_master]
                        set tb3_master [$db findMaster $tap_nwout3_master]
                        set tbtie_master [$db findMaster $tap_nwouttie_master]
                    }

                    set res {}
                    foreach item $endcap_xs {
                        if {$item ni $endcap_above_xs} {
                            lappend res $item
                        }
                    }

                    #puts $res
                    for {set i 0} {$i < [llength $res]} {set i [expr $i+2]} {
                        set x_start [lindex $res $i]
                        set x_end [lindex $res $i+1]

                        #insert above row
                        set lly [expr $y + $site_y]

                        #insert incnr
                        set inst1_name "PHY_${cnt}"
                        set inst1 [odb::dbInst_create $block $incnr_master $inst1_name]
                        $inst1 setOrient $ori
                        $inst1 setLocation $x_start $lly
                        $inst1 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt

                        set inst2_name "PHY_${cnt}"
                        set inst2 [odb::dbInst_create $block $incnr_master $inst2_name]
                        $inst2 setOrient $ori
                        $inst2 setLocation $x_end $lly
                        $inst2 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt

                        for {set x [expr $x_start+$endcapwidth]} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                            set inst3_name "PHY_${cnt}"
                            set inst3 [odb::dbInst_create $block $tbtie_master $inst3_name]
                            $inst3 setOrient $ori
                            $inst3 setLocation $x $lly
                            $inst3 setPlacementStatus LOCKED

                            incr cnt
                            incr blkgs_cnt
                        }

                        set numcpp [format %i [expr int(($x_end - $x)/$site_x)]]

                        if {[expr $numcpp % 2] == 1} {
                            set x_tb3 [expr $x_end-(3*$site_x)]
                            set inst4_name "PHY_${cnt}"
                            set inst4 [odb::dbInst_create $block $tb3_master $inst4_name]
                            $inst4 setOrient $ori
                            $inst4 setLocation $x_tb3 $lly
                            $inst4 setPlacementStatus LOCKED

                            incr cnt
                            incr blkgs_cnt
                            set x_end $x_tb3
                        }

                        for {} {$x < $x_end} {set x [expr $x+(2*$site_x)]} {
                            set inst5_name "PHY_${cnt}"
                            set inst5 [odb::dbInst_create $block $tb2_master $inst5_name]
                            $inst5 setOrient $ori
                            $inst5 setLocation $x $lly

                            incr cnt
                            incr blkgs_cnt
                        }
                    }
                }
            }
        }
        puts "Cells inserted near blkgs: $blkgs_cnt"
    }

    puts "Running tapcell... Done!"
}