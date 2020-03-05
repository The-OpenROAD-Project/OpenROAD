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
                if {$row_lly < $blockage_lly} {
                    return -1
                } else {
                    return 1
                }
            }

            if {[expr ($dx < 0) && ($dy_below < 0)]} {
                if {$row_lly < $blockage_lly} {
                    return -1
                } else {
                    return 1
                }
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

    proc get_insts_in_area {ll_x ll_y ur_x ur_y block} {
        set ll_x [expr { int($ll_x) }]
        set ll_y [expr { int($ll_y) }]
        set ur_x [expr { int($ur_x) }]
        set ur_y [expr { int($ur_y) }]

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

    proc get_rows_top_bottom_macro {macro rows halo_x halo_y} {
        set macro_llx [expr [[$macro getBBox] xMin] - $halo_x]
        set macro_lly [expr [[$macro getBBox] yMin] - $halo_y]
        set macro_urx [expr [[$macro getBBox] xMax] + $halo_x]
        set macro_ury [expr [[$macro getBBox] yMax] + $halo_y]

        set top_bottom_rows ""

        foreach row $rows {
            set row_llx [[$row getBBox] xMin]
            set row_lly [[$row getBBox] yMin]
            set row_urx [[$row getBBox] xMax]
            set row_ury [[$row getBBox] yMax]

            set row_height [expr $row_ury - $row_lly]

            set row_below_ury [expr $row_ury + $row_height]
            set row_above_lly [expr $row_lly - $row_height]

            set min_x [expr max($macro_llx, $row_llx)]
            set max_x [expr min($macro_urx, $row_urx)]

            set min_above_y [expr max($macro_lly, $row_above_lly)]
            set max_below_y [expr min($macro_ury, $row_below_ury)]
            
            set min_y [expr max($macro_lly, $row_lly)]
            set max_y [expr min($macro_ury, $row_ury)]

            set dx [expr $min_x - $max_x]
            
            set dy_above [expr $min_above_y - $max_y]
            set dy_below [expr $min_y - $max_below_y]

            if {[expr ($dx < 0) && ($dy_above < 0)]} {
                lappend top_bottom_rows $row
            } elseif {[expr ($dx < 0) && ($dy_below < 0)]} {
                lappend top_bottom_rows $row
            }
        }

        return $top_bottom_rows
    }

    proc get_macros_top_bottom_row {row macros halo_x halo_y} {
        set row_llx [[$row getBBox] xMin]
        set row_lly [[$row getBBox] yMin]
        set row_urx [[$row getBBox] xMax]
        set row_ury [[$row getBBox] yMax]

        set top_bottom_macros ""

        foreach macro $macros {
            set macro_llx [expr [[$macro getBBox] xMin] - $halo_x]
            set macro_lly [expr [[$macro getBBox] yMin] - $halo_y]
            set macro_urx [expr [[$macro getBBox] xMax] + $halo_x]
            set macro_ury [expr [[$macro getBBox] yMax] + $halo_y]
            set row_height [expr $row_ury - $row_lly]

            set row_below_ury [expr $row_ury + $row_height]
            set row_above_lly [expr $row_lly - $row_height]

            set min_x [expr max($macro_llx, $row_llx)]
            set max_x [expr min($macro_urx, $row_urx)]

            set min_above_y [expr max($macro_lly, $row_above_lly)]
            set max_below_y [expr min($macro_ury, $row_below_ury)]
            
            set min_y [expr max($macro_lly, $row_lly)]
            set max_y [expr min($macro_ury, $row_ury)]

            set dx [expr $min_x - $max_x]
            
            set dy_above [expr $min_above_y - $max_y]
            set dy_below [expr $min_y - $max_below_y]

            if {[expr ($dx < 0) && ($dy_above < 0)]} {
                lappend top_bottom_macros $macro
            } elseif {[expr ($dx < 0) && ($dy_below < 0)]} {
                lappend top_bottom_macros $macro
            }
        }

        return $top_bottom_macros
    }

    proc in_blocked_region {x row blockages halo_x halo_y master_width endcapwidth} {
        set row_blockages ""
        set row_blockages [get_macros_top_bottom_row $row $blockages $halo_x $halo_y]
        
        set blocked_region false

        if {([llength $row_blockages] > 0)} {
            foreach row_blockage $row_blockages {
                set row_blockage_llx [expr [[$row_blockage getBBox] xMin] - $halo_x]
                set row_blockage_urx [expr [[$row_blockage getBBox] xMax] + $halo_x]
                if {($x + $master_width) >= ($row_blockage_llx - $endcapwidth) && \
                     $x <= ($row_blockage_urx + $endcapwidth)} {
                    return true
                }
            }
        }

        return false
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
    set tech [$db getTech]
    set lef_units [$tech getLefUnits]

    set halo_y [expr $halo_y * $lef_units]
    set halo_x [expr $halo_x * $lef_units]

    #Step 1: cut placement rows if there are overlaps between rows and placement blockages
    puts "Step 1: Cut rows..."

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
    puts "Step 2: Insert endcaps..."
    
    set rows [$block getRows]
    
    set min_y [tapcell::get_min_rows_y $rows]
    set max_y [tapcell::get_max_rows_y $rows]

    set cnt 0
    set endcap_count 0
    foreach row $rows {
        set master [$db findMaster $endcap_master]
        set site_x [[$row getSite] getWidth]
        set endcapwidth [expr $endcap_cpp*$site_x]

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

        set blocked_region false
        set blocked_region [tapcell::in_blocked_region $llx $row $blockages $halo_x $halo_y [$master getWidth] $endcapwidth]
        if {$add_boundary_cell == true && $blocked_region == true} {    
            if {[tapcell::right_above_below_macros $blockages $row $halo_x $halo_y] == 1} {
                if {[string match "MX" $ori]} {
                    set master [$db findMaster $cnrcap_nwin_master]
                } else {
                    set master [$db findMaster $cnrcap_nwout_master]
                }
            } else {
                if {[string match "R0" $ori]} {
                    set master [$db findMaster $cnrcap_nwin_master]
                } else {
                    set master [$db findMaster $cnrcap_nwout_master]
                }
            }
        }

        set inst1_name "PHY_${cnt}"
        set inst1 [odb::dbInst_create $block $master $inst1_name]
        $inst1 setOrient $ori
        $inst1 setLocation $llx $lly
        $inst1 setPlacementStatus LOCKED

        incr cnt
        incr endcap_count

        set blocked_region false
        set blocked_region [tapcell::in_blocked_region $loc_2_x $row $blockages $halo_x $halo_y [$master getWidth] $endcapwidth]
        if {$add_boundary_cell == true && $blocked_region == true} {
            if {[tapcell::right_above_below_macros $blockages $row $halo_x $halo_y] == 1} {
                if {[string match "MX" $ori]} {
                    set master [$db findMaster $cnrcap_nwin_master]
                } else {
                    set master [$db findMaster $cnrcap_nwout_master]
                }
            } else {
                if {[string match "R0" $ori]} {
                    set master [$db findMaster $cnrcap_nwin_master]
                } else {
                    set master [$db findMaster $cnrcap_nwout_master]
                }
            }
        }

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
    puts "Step 3: Insert tapcells..."

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
        } elseif {[tapcell::right_above_below_macros $blockages $row $halo_x $halo_y] && \
                  $add_boundary_cell == false} {
            set pitch [expr $dist*$lef_units]
            set offset [expr $dist*$lef_units]
        } else {
            set pitch [expr $dist*2*$lef_units]
        }

        set endcapwidth [expr $endcap_cpp*$site_x]
        for {set x [expr $llx+$offset]} {$x < [expr $urx-$endcap_cpp*$site_x]} {set x [expr $x+$pitch]} {
            set master [$db findMaster $tapcell_master]
            set inst_name "PHY_${cnt}"

            if { [string match [$master getConstName] $tapcell_master] } {
                if {$add_boundary_cell == true} {
                    set blocked_region false
                    set blocked_region [tapcell::in_blocked_region $x $row $blockages $halo_x $halo_y [$master getWidth] $endcapwidth]
                    if {$blocked_region == true} {
                        continue
                    }
                }

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
        puts "Step 4.1: Insert tapcells at top/bottom between cnr cell..."

        set tbtiewidth [expr $tbtie_cpp*$site_x]
        set endcapwidth [expr $endcap_cpp*$site_x]

        set topbottom_cnt 0

        foreach row $rows {
            set site_x [[$row getSite] getWidth]

            set llx [[$row getBBox] xMin]
            set lly [[$row getBBox] yMin]
            set urx [[$row getBBox] xMax]
            set ury [[$row getBBox] yMax]

            set ori [$row getOrient]

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

        puts "---- Top/bottom cells inserted: $topbottom_cnt"

        #Step 4-2: insert incnr/topbottom for blkgs
        puts "Step 4.2: Insert tapcells incnr/top/bottom for blkgs..."

        set rows [$block getRows]

        set corebox_llx [tapcell::get_min_rows_x $rows]
        set corebox_lly [tapcell::get_min_rows_y $rows]
        set corebox_urx [tapcell::get_max_rows_x $rows]
        set corebox_ury [tapcell::get_max_rows_y $rows]

        set blkgs_cnt 0      

        foreach blockage $blockages {
            set blockage_llx [expr [[$blockage getBBox] xMin] - $halo_x]
            set blockage_lly [expr [[$blockage getBBox] yMin] - $halo_y]
            set blockage_urx [expr [[$blockage getBBox] xMax] + $halo_x]
            set blockage_ury [expr [[$blockage getBBox] yMax] + $halo_y]

            set rows_top_bottom [tapcell::get_rows_top_bottom_macro $blockage $rows $halo_x $halo_y]
            
            foreach row $rows_top_bottom {
                set site_x [[$row getSite] getWidth]

                set ori [$row getOrient]
                set row_llx [[$row getBBox] xMin]
                set row_lly [[$row getBBox] yMin]
                set row_urx [[$row getBBox] xMax]
                set row_ury [[$row getBBox] yMax]

                if {($row_lly >= $blockage_ury)} {
                    # If row is at top of macro
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

                    # Insert incnr cells
                    set row1_num_sites [expr {($blockage_llx - $row_llx)/$site_x}]
                    
                    set x_tmp [expr {$row_llx + ($row1_num_sites * $site_x) - [$incnr_master getWidth]}]
                    #set x_tmp [expr {round (1.0*$x_start/$site_x)*$site_x}]
                    set x_start [expr { int($x_tmp) }]
                    
                    set x_end [expr $blockage_urx]
                    set x_tmp [expr {ceil (1.0*$x_end/$site_x)*$site_x}]
                    set x_end [expr { int($x_tmp) }]

                    set inst1_name "PHY_${cnt}"
                    set inst1 [odb::dbInst_create $block $incnr_master $inst1_name]
                    $inst1 setOrient $ori
                    $inst1 setLocation $x_start $row_lly
                    $inst1 setPlacementStatus LOCKED

                    incr cnt
                    incr blkgs_cnt

                    if {(($x_end + $endcapwidth) < $corebox_urx)} {
                        set inst2_name "PHY_${cnt}"
                        set inst2 [odb::dbInst_create $block $incnr_master $inst2_name]
                        $inst2 setOrient $ori
                        $inst2 setLocation $x_end $row_lly
                        $inst2 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                    } else {
                        set x_end [expr $corebox_urx - $endcapwidth]
                    }

                    #Insert remaining cells
                    for {set x [expr $x_start+$endcapwidth]} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                        set inst3_name "PHY_${cnt}"
                        set inst3 [odb::dbInst_create $block $tbtie_master $inst3_name]
                        $inst3 setOrient $ori
                        $inst3 setLocation $x $row_lly
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
                        $inst4 setLocation $x_tb3 $row_lly
                        $inst4 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                        set x_end $x_tb3
                    }

                    for {set x $x} {$x < $x_end} {set x [expr $x+(2*$site_x)]} {
                        set inst5_name "PHY_${cnt}"
                        set inst5 [odb::dbInst_create $block $tb2_master $inst5_name]
                        $inst5 setOrient $ori
                        $inst5 setLocation $x $row_lly
                        $inst5 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                    }
                } elseif {($row_ury <= $blockage_lly)} {
                    # If row is at bottom of macro
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

                    # Insert incnr cells
                    set row1_num_sites [expr {($blockage_llx - $row_llx)/$site_x}]
                    
                    set x_tmp [expr {$row_llx + ($row1_num_sites * $site_x) - [$incnr_master getWidth]}]
                    #set x_tmp [expr {round (1.0*$x_start/$site_x)*$site_x}]
                    set x_start [expr { int($x_tmp) }]
                    
                    set x_end [expr $blockage_urx]
                    set x_tmp [expr {ceil (1.0*$x_end/$site_x)*$site_x}]
                    set x_end [expr { int($x_tmp) }]

                    set inst1_name "PHY_${cnt}"
                    set inst1 [odb::dbInst_create $block $incnr_master $inst1_name]
                    $inst1 setOrient $ori
                    $inst1 setLocation $x_start $row_lly
                    $inst1 setPlacementStatus LOCKED

                    incr cnt
                    incr blkgs_cnt

                    if {(($x_end + $endcapwidth) < $corebox_urx)} {
                        set inst2_name "PHY_${cnt}"
                        set inst2 [odb::dbInst_create $block $incnr_master $inst2_name]
                        $inst2 setOrient $ori
                        $inst2 setLocation $x_end $row_lly
                        $inst2 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                    } else {
                        set x_end [expr $corebox_urx - $endcapwidth]
                    }
                    
                    #Insert remaining cells
                    for {set x [expr $x_start+$endcapwidth]} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                        set inst3_name "PHY_${cnt}"
                        set inst3 [odb::dbInst_create $block $tbtie_master $inst3_name]
                        $inst3 setOrient $ori
                        $inst3 setLocation $x $row_lly
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
                        $inst4 setLocation $x_tb3 $row_lly
                        $inst4 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                        set x_end $x_tb3
                    }

                    for {} {$x < $x_end} {set x [expr $x+(2*$site_x)]} {
                        set inst5_name "PHY_${cnt}"
                        set inst5 [odb::dbInst_create $block $tb2_master $inst5_name]
                        $inst5 setOrient $ori
                        $inst5 setLocation $x $row_lly
                        $inst5 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                    }
                }
            }
        }
        puts "---- Cells inserted near blkgs: $blkgs_cnt"
    }

    puts "Running tapcell... Done!"
}
