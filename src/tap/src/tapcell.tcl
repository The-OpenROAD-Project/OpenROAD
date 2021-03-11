###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, University of California, San Diego.
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
##
###############################################################################


sta::define_cmd_args "tapcell" {[-tapcell_master tapcell_master]\
                                [-endcap_master endcap_master]\
                                [-endcap_cpp endcap_cpp]\
                                [-distance dist]\
                                [-halo_width_x halo_x]\
                                [-halo_width_y halo_y]\
                                [-tap_nwin2_master tap_nwin2_master]\
                                [-tap_nwin3_master tap_nwin3_master]\
                                [-tap_nwout2_master tap_nwout2_master]\
                                [-tap_nwout3_master tap_nwout3_master]\
                                [-tap_nwintie_master tap_nwintie_master]\
                                [-tap_nwouttie_master tap_nwouttie_master]\
                                [-cnrcap_nwin_master cnrcap_nwin_master]\
                                [-cnrcap_nwout_master cnrcap_nwout_master]\
                                [-incnrcap_nwin_master incnrcap_nwin_master]\
                                [-incnrcap_nwout_master incnrcap_nwout_master]\
                                [-tbtie_cpp tbtie_cpp]\
                                [-no_cell_at_top_bottom]\
                                [-add_boundary_cell]\
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

    set tap_nwin2_master "INVALID"
    if { [info exists keys(-tap_nwin2_master)] } {
        set tap_nwin2_master $keys(-tap_nwin2_master)
    }

    set tap_nwin3_master "INVALID"
    if { [info exists keys(-tap_nwin3_master)] } {
        set tap_nwin3_master $keys(-tap_nwin3_master)
    }

    set tap_nwout2_master "INVALID"
    if { [info exists keys(-tap_nwout2_master)] } {
        set tap_nwout2_master $keys(-tap_nwout2_master)
    }

    set tap_nwout3_master "INVALID"
    if { [info exists keys(-tap_nwout3_master)] } {
        set tap_nwout3_master $keys(-tap_nwout3_master)
    }

    set tap_nwintie_master "INVALID"
    if { [info exists keys(-tap_nwintie_master)] } {
        set tap_nwintie_master $keys(-tap_nwintie_master)
    }

    set tap_nwouttie_master "INVALID"
    if { [info exists keys(-tap_nwouttie_master)] } {
        set tap_nwouttie_master $keys(-tap_nwouttie_master)
    }

    set cnrcap_nwin_master "INVALID"
    if { [info exists keys(-cnrcap_nwin_master)] } {
        set cnrcap_nwin_master $keys(-cnrcap_nwin_master)
    }

    set cnrcap_nwout_master "INVALID"
    if { [info exists keys(-cnrcap_nwout_master)] } {
        set cnrcap_nwout_master $keys(-cnrcap_nwout_master)
    }

    set incnrcap_nwin_master "INVALID"
    if { [info exists keys(-incnrcap_nwin_master)] } {
        set incnrcap_nwin_master $keys(-incnrcap_nwin_master)
    }

    set incnrcap_nwout_master "INVALID"
    if { [info exists keys(-incnrcap_nwout_master)] } {
        set incnrcap_nwout_master $keys(-incnrcap_nwout_master)
    }

    if { [info exists keys(-tbtie_cpp)] } {
        set tbtie_cpp $keys(-tbtie_cpp)
    }

    set add_boundary_cell [info exists flags(-add_boundary_cell)]
    set no_cell_at_top_bottom [info exists flags(-no_cell_at_top_bottom)]
        
    set db [ord::get_db]
    set block [[$db getChip] getBlock]

    set halo_y [ord::microns_to_dbu $halo_y]
    set halo_x [ord::microns_to_dbu $halo_x]

    set blockages [tap::find_blockages $db]
    
    set cnrcap_masters {}
    lappend cnrcap_masters $cnrcap_nwin_master $cnrcap_nwout_master

    tap::cut_rows $db $endcap_master $blockages $halo_x $halo_y

    set top_bottom_blockages [tap::get_macros_top_bottom_rows $db $blockages $halo_x $halo_y]
    set overlapping_blockages [tap::get_macros_overlapping_rows $db $blockages $halo_x $halo_y]

    set cnt [tap::insert_endcaps $db $endcap_cpp $endcap_master \
                                     $cnrcap_masters \
                                     $blockages $top_bottom_blockages $halo_x $halo_y \
                                     $no_cell_at_top_bottom \
                                     $add_boundary_cell]

    set endcap_master [$db findMaster $endcap_master]
    set endcap_width [$endcap_master getWidth]
    set cnt [tap::insert_tapcells $db $tapcell_master $blockages $overlapping_blockages $top_bottom_blockages $dist \
                                      $endcap_cpp $halo_x $halo_y \
                                      $endcap_width $cnt $no_cell_at_top_bottom \
                                      $add_boundary_cell]

    if {$add_boundary_cell} {
        set tap_nw_masters {}
        lappend tap_nw_masters $tap_nwintie_master $tap_nwin2_master $tap_nwin3_master \
                               $tap_nwouttie_master $tap_nwout2_master $tap_nwout3_master

        set tap_macro_masters {}
        lappend tap_macro_masters $incnrcap_nwin_master $tap_nwin2_master $tap_nwin3_master \
                                  $tap_nwintie_master $incnrcap_nwout_master $tap_nwout2_master \
                                  $tap_nwout3_master $tap_nwouttie_master

        set cnt [tap::insert_at_top_bottom $db $tap_nw_masters $tbtie_cpp $endcap_cpp $cnt]
        tap::insert_around_macros $db $tap_macro_masters $blockages $cnt $halo_x $halo_y $endcap_width $tbtie_cpp
    }
}

namespace eval tap {
    proc cut_rows {db endcap_master blockages halo_x halo_y} {
        set block [[$db getChip] getBlock]

        set rows_count 0
        foreach row [$block getRows] {
            incr rows_count
        }

        set end_master [$db findMaster $endcap_master]
        if { $end_master == "NULL" } {
            utl::error TAP 10 "Master $endcap_master not found."
        }
        set end_width [$end_master getWidth]
        set min_row_width [expr 2*$end_width]

        set block_count 0
        set cut_rows_count 0
        foreach blockage $blockages {
            set rows [$block getRows]
            incr block_count
            foreach row $rows {
                if {[overlaps $blockage $row $halo_x $halo_y]} {
                    incr cut_rows_count
                    # Create two new rows, avoiding overlap with blockage
                    set row_site [$row getSite]
                    set orient [$row getOrient]
                    set direction [$row getDirection]

                    set site_width [$row_site getWidth]

                    set row1_name [$row getName]
                    set row2_name [$row getName]

                    append row1_name "_1"
                    append row2_name "_2"

                    set rowBB [$row getBBox]
                    ## First new row: from left of original row to the left boundary of blockage
                    set row1_origin_x [$rowBB xMin]
                    set row1_origin_y [$rowBB yMin]
                    set row1_end_x [expr [$blockage xMin] - $halo_x]
                    set row1_num_sites [expr {($row1_end_x - $row1_origin_x)/$site_width}]

                    set curr_min_row_width [expr $min_row_width + 2*$site_width]
                    set row1_width [expr $row1_num_sites*$site_width]

                    if {$row1_num_sites > 0 && $row1_width >= $curr_min_row_width} {
                        odb::dbRow_create $block $row1_name $row_site $row1_origin_x $row1_origin_y $orient $direction $row1_num_sites $site_width
                    }

                    ## Second new row: from right of original  row to the right boundary of blockage
                    set blockage_x_max [$blockage xMax]

                    set row2_origin_x_tmp [expr {ceil (1.0*($blockage_x_max + $halo_x)/$site_width)*$site_width}]
                    set row2_origin_x [expr { int($row2_origin_x_tmp) }]
                    set row2_origin_y [$rowBB yMin]
                    set row2_end_x [$rowBB xMax]
                    set row2_num_sites [expr {($row2_end_x - $row2_origin_x)/$site_width}]

                    set row2_width [expr $row2_num_sites*$site_width]

                    if {$row2_num_sites > 0 && $row2_width >= $curr_min_row_width} {
                        odb::dbRow_create $block $row2_name $row_site $row2_origin_x $row2_origin_y $orient $direction $row2_num_sites $site_width
                    }

                    # Remove current row
                    odb::dbRow_destroy $row
                }
            }
        }

        utl::info TAP 1 "Macro blocks found: $block_count"
        utl::info TAP 2 "Original rows: $rows_count"
        utl::info TAP 3 "Cut rows: $cut_rows_count"
    }

    proc insert_endcaps {db endcap_cpp endcap_master cnrcap_masters \
                         blockages top_bottom_blockages halo_x halo_y no_cell_at_top_bottom add_boundary_cell} {
        set block [[$db getChip] getBlock]
        
        set rows [$block getRows]
        
        set min_y [get_min_rows_y $rows]
        set max_y [get_max_rows_y $rows]

        if {[llength $cnrcap_masters] == 2} {
            lassign $cnrcap_masters cnrcap_nwin_master cnrcap_nwout_master
        }

        set cnt 0
        set endcap_count 0
        set row_idx -1
        foreach row $rows {
            incr row_idx
            set master [$db findMaster $endcap_master]
            set site_x [[$row getSite] getWidth]
            set endcapwidth [expr $endcap_cpp*$site_x]

            if { $master == "NULL" } {
                utl::error TAP 11 "Master $endcap_master not found."
            }

            set row_name [$row getName]

            set ori [$row getOrient]

            set top_bottom [top_or_bottom $row $min_y $max_y]

            if {$no_cell_at_top_bottom} {
                if {$top_bottom == 1} {
                    if { $ori == "MX" } {
                        set master [$db findMaster $cnrcap_nwin_master]

                        if { $master == "NULL" } {
                            utl::error TAP 12 "Master $cnrcap_nwin_master not found."
                        }
                    } else {
                        set master [$db findMaster $cnrcap_nwout_master]
                        
                        if { $master == "NULL" } {
                            utl::error TAP 13 "Master $cnrcap_nwout_master not found."
                        }
                    }
                } elseif {$top_bottom == -1} {
                    if { $ori == "R0" } {
                        set master [$db findMaster $cnrcap_nwin_master]

                        if { $master == "NULL" } {
                            utl::error TAP 14 "Master $cnrcap_nwin_master not found."
                        }
                    } else {
                        set master [$db findMaster $cnrcap_nwout_master]

                        if { $master == "NULL" } {
                            utl::error TAP 15 "Master $cnrcap_nwout_master not found."
                        }
                    }
                } else {
                    set master [$db findMaster $endcap_master]
                }
            }

            set master_x [$master getWidth]
            set master_y [$master getHeight]

            set rowBB [$row getBBox]
            set llx [$rowBB xMin]
            set lly [$rowBB yMin]
            set urx [$rowBB xMax]
            set ury [$rowBB yMax]

            set row_width [expr $urx - $llx]
            
            if {$master_x > $row_width} {
                continue
            }

            set loc_2_x [expr $urx - $master_x]
            set loc_2_y [expr $ury - $master_y]

            set row_top_bottom_blockages [lindex $top_bottom_blockages $row_idx]
            set blocked_region [in_blocked_region $llx $row $row_top_bottom_blockages $halo_x $halo_y [$master getWidth] $endcapwidth]
            if {$add_boundary_cell && $blocked_region} {    
                if {[right_above_below_macros $row_top_bottom_blockages $row $halo_x $halo_y] == 1} {
                    if { $ori == "MX" } {
                        set master [$db findMaster $cnrcap_nwin_master]
                    } else {
                        set master [$db findMaster $cnrcap_nwout_master]
                    }
                } else {
                    if { $ori == "R0" } {
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

            set blocked_region [in_blocked_region $loc_2_x $row $row_top_bottom_blockages $halo_x $halo_y [$master getWidth] $endcapwidth]
            if {$add_boundary_cell && $blocked_region} {
                if {[right_above_below_macros $row_top_bottom_blockages $row $halo_x $halo_y] == 1} {
                    if { $ori == "MX" } {
                        set master [$db findMaster $cnrcap_nwin_master]
                    } else {
                        set master [$db findMaster $cnrcap_nwout_master]
                    }
                } else {
                    if { $ori == "R0" } {
                        set master [$db findMaster $cnrcap_nwin_master]
                    } else {
                        set master [$db findMaster $cnrcap_nwout_master]
                    }
                }
            }

            if {$llx == $loc_2_x && $lly == $loc_2_y} {
                utl::warn TAP 9 "row $row_name have enough space for only one endcap."
            continue
            }

            set inst2_name "PHY_${cnt}"
            set inst2 [odb::dbInst_create $block $master $inst2_name]
            set right_ori $ori
            if { $ori == "MX" } {
                set right_ori "R180"
            } else {
                if { $ori == "R0"} {
                    set right_ori "MY"
                }
            }
            $inst2 setOrient $right_ori
            $inst2 setLocation $loc_2_x $loc_2_y
            $inst2 setPlacementStatus LOCKED

            incr cnt
            incr endcap_count
        }
        utl::info TAP 4 "Endcaps inserted: $endcap_count"
        return $cnt
    }

    proc insert_tapcells {db tapcell_master blockages overlapping_blockages top_bottom_blockages dist endcap_cpp halo_x halo_y \
                          endcap_width cnt no_cell_at_top_bottom add_boundary_cell} {
        set block [[$db getChip] getBlock]
        
        set rows [$block getRows]

        set min_x [get_min_rows_x $rows]
        set max_x [get_max_rows_x $rows]

        set min_y [get_min_rows_y $rows]
        set max_y [get_max_rows_y $rows]
        
        set tapcell_count 0

        set first_row_orient "R0"
        foreach row $rows {
            if {[top_or_bottom $row $min_y $max_y] == -1} {
                set first_row_orient [$row getOrient]
                break
            }
        }

        set row_idx -1
        foreach row $rows {
            incr row_idx
            set site_x [[$row getSite] getWidth]
            set llx [[$row getBBox] xMin]
            set lly [[$row getBBox] yMin]
            set urx [[$row getBBox] xMax]
            set ury [[$row getBBox] yMax]

            set ori [$row getOrient]

            set offsets ""
            set pitch -1

            if {[even $row $first_row_orient]} {
                set offset [ord::microns_to_dbu $dist]
                lappend offsets $offset
            } else {
                set offset [ord::microns_to_dbu [expr $dist*2]]
                lappend offsets $offset
            }

            set row_top_bottom_blockages [lindex $top_bottom_blockages $row_idx]
            set row_overlap_blockages [lindex $overlapping_blockages $row_idx]

            if {[top_or_bottom $row $min_y $max_y]} {
                if {$no_cell_at_top_bottom} {
                    continue
                }
                set offsets ""
                set pitch [ord::microns_to_dbu $dist]
                set offset [ord::microns_to_dbu $dist]
                lappend offsets $offset
            } elseif {[right_above_below_macros $row_top_bottom_blockages $row $halo_x $halo_y]} {
                if {$add_boundary_cell} {
                    set offsets ""
                    set pitch [ord::microns_to_dbu [expr $dist*2]]
                    set offset [ord::microns_to_dbu $dist]
                    set offset2 [ord::microns_to_dbu [expr $dist*2]]
                    lappend offsets $offset
                    lappend offsets $offset2
                } else {
                    set offsets ""
                    set offset [ord::microns_to_dbu $dist]
                    lappend offsets $offset
                    set pitch [ord::microns_to_dbu $dist]
                }
            } else {
                set pitch [ord::microns_to_dbu [expr $dist*2]]
            }

            set endcapwidth [expr $endcap_cpp*$site_x]
            foreach offset $offsets {
                for {set x [expr $llx+$offset]} {$x < [expr $urx-$endcap_cpp*$site_x]} {set x [expr $x+$pitch]} {
                    set master [$db findMaster $tapcell_master]
                    if { $master == "NULL" } {
                        utl::error TAP 16 "Master $tapcell_master not found."
                    }

                    set inst_name "PHY_${cnt}"

                    set x_tmp [expr {floor (1.0*$x/$site_x)*$site_x}]
                    set row_orig_fix [expr { $llx % $site_x }]
                    set x [expr { int($x_tmp + $row_orig_fix) }]
                    set x_end [expr $x + $site_x]
                    set x_tmp $x

                    if {$add_boundary_cell} {
                        set blocked_region [in_blocked_region $x $row $row_top_bottom_blockages $halo_x $halo_y [$master getWidth] $endcapwidth]
                        if {$blocked_region} {
                            set new_x [get_new_x $x $row $row_top_bottom_blockages $halo_x $halo_y [$master getWidth] $endcapwidth $site_x]
                            if { $x != $new_x} {
                                set x_tmp $new_x
                            } else {
                                continue
                            }
                        }
                    }

                    set tap_width [$master getWidth]
                    set tap_urx [expr $x_tmp + $tap_width]
                    set end_llx [expr $urx - $endcap_width]

                    if {($x_tmp != $min_x) && ($x_end != $max_x)} {
                        if { $tap_urx > $end_llx } {
                            set x_microns [ord::dbu_to_microns $x_tmp]
                            set y_microns [ord::dbu_to_microns $lly]
                            set x_tmp [expr $x_tmp - ($tap_urx - $end_llx)]
                        }

                        set new_x [expr {floor (1.0*$x_tmp/$site_x)*$site_x}]
                        set new_x [expr { int($new_x) }]
                        set real_x [get_correct_llx $new_x $row $row_overlap_blockages $halo_x $halo_y [$master getWidth] $endcapwidth $site_x $add_boundary_cell]

                        set inst [odb::dbInst_create $block $master $inst_name]
                        $inst setOrient $ori

                        $inst setLocation $real_x $lly
                        $inst setPlacementStatus LOCKED

                        incr cnt
                        incr tapcell_count
                    }
                }
            }
        }

        utl::info TAP 5 "Tapcells inserted: $tapcell_count"
        return $cnt
    }

    proc insert_at_top_bottom {db masters tbtie_cpp endcap_cpp cnt} {
        set block [[$db getChip] getBlock]
        set rows [$block getRows]

        set topbottom_cnt 0

        set min_y [get_min_rows_y $rows]
        set max_y [get_max_rows_y $rows]

        if {[llength $masters] == 6} {
            lassign $masters tap_nwintie_master tap_nwin2_master tap_nwin3_master \
                             tap_nwouttie_master tap_nwout2_master tap_nwout3_master
        }

        foreach row $rows {
            set site_x [[$row getSite] getWidth]
            set tbtiewidth [expr $tbtie_cpp*$site_x]
            set endcapwidth [expr $endcap_cpp*$site_x]

            set llx [[$row getBBox] xMin]
            set lly [[$row getBBox] yMin]
            set urx [[$row getBBox] xMax]
            set ury [[$row getBBox] yMax]

            set ori [$row getOrient]

            set topbottom_chk [top_or_bottom $row $min_y $max_y]
            
            set master NULL
            set tb2_master NULL
            set tb3_master NULL

            if {$topbottom_chk == 1} {
                # top
                if { $ori == "MX" } {
                    set master [$db findMaster $tap_nwintie_master]
                    set tb2_master [$db findMaster $tap_nwin2_master]
                    set tb3_master [$db findMaster $tap_nwin3_master]
                } else {
                    set master [$db findMaster $tap_nwouttie_master]
                    set tb2_master [$db findMaster $tap_nwout2_master]
                    set tb3_master [$db findMaster $tap_nwout3_master]
                }
            } elseif {$topbottom_chk == -1} {
                # bottom
                if { $ori == "R0" } {
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
                set x_tb2 $x_start
                for {set x $x_start} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                    set inst_name "PHY_${cnt}"
                    set tie_diff [expr $x_end-($x+$tbtiewidth)]
                    set sites_gap [expr $tie_diff/$site_x]
                    if {$sites_gap <= 1} {
                        continue
                    }
                    set new_inst [odb::dbInst_create $block $master $inst_name]
                    $new_inst setOrient $ori
                    $new_inst setLocation $x $lly
                    $new_inst setPlacementStatus LOCKED

                    incr cnt
                    incr topbottom_cnt
                    set x_tb2 [expr $x+$tbtiewidth]
                }

                set numcpp [format %i [expr int(($x_end - $x)/$site_x)]]
                if {[expr $numcpp % 2] == 1} {
                    set inst_name "PHY_${cnt}"
                    set x_tb3 [expr $x_end-(3*$site_x)]
                    if {$x_tb3 >= $llx && $x_tb3+(3*$site_x) <= $urx} {
                        set new_inst [odb::dbInst_create $block $tb3_master $inst_name]
                        $new_inst setOrient $ori
                        $new_inst setLocation $x_tb3 $lly
                        $new_inst setPlacementStatus LOCKED

                        incr cnt
                        incr topbottom_cnt
                        set x_end $x_tb3
                    }
                }

                for {} {$x_tb2 < $x_end} {set x_tb2 [expr $x_tb2+(2*$site_x)]} {
                    set inst_name "PHY_${cnt}"
                    set new_inst [odb::dbInst_create $block $tb2_master $inst_name]
                    $new_inst setOrient $ori
                    $new_inst setLocation $x_tb2 $lly
                    $new_inst setPlacementStatus LOCKED

                    incr cnt
                    incr topbottom_cnt
                }
            }
        }

        utl::info TAP 6 "Top/bottom cells inserted: $topbottom_cnt"
        return $cnt
    }

    proc insert_around_macros {db masters blockages cnt halo_x halo_y endcapwidth tbtie_cpp} {
        set block [[$db getChip] getBlock]
        set rows [$block getRows]

        set corebox_llx [get_min_rows_x $rows]
        set corebox_lly [get_min_rows_y $rows]
        set corebox_urx [get_max_rows_x $rows]
        set corebox_ury [get_max_rows_y $rows]

        set blkgs_cnt 0

        if {[llength $masters] == 8} {
            lassign $masters incnrcap_nwin_master tap_nwin2_master tap_nwin3_master tap_nwintie_master \
                             incnrcap_nwout_master tap_nwout2_master tap_nwout3_master tap_nwouttie_master
        }

        foreach blockage $blockages {
            set blockage_llx_ [expr [$blockage xMin] - $halo_x]
            set blockage_lly [expr [$blockage yMin] - $halo_y]
            set blockage_urx_ [expr [$blockage xMax] + $halo_x]
            set blockage_ury [expr [$blockage yMax] + $halo_y]

            set rows_top_bottom [get_rows_top_bottom_macro $blockage $rows $halo_x $halo_y]
            
            foreach row $rows_top_bottom {
                set site_x [[$row getSite] getWidth]
                set tbtiewidth [expr $tbtie_cpp*$site_x]

                set ori [$row getOrient]
                set row_llx [[$row getBBox] xMin]
                set row_lly [[$row getBBox] yMin]
                set row_urx [[$row getBBox] xMax]
                set row_ury [[$row getBBox] yMax]

                if {$blockage_llx_ < $row_llx} {
                    set blockage_llx $row_llx
                } else {
                    set blockage_llx $blockage_llx_
                }

                if {$blockage_urx_ > $row_urx} {
                    set blockage_urx $row_urx
                } else {
                    set blockage_urx $blockage_urx_
                }

                if {($row_lly >= $blockage_ury)} {
                    # If row is at top of macro
                    if { $ori == "R0" } {
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

                    set cell_orient $ori

                    # Insert incnr cells
                    set row1_num_sites [expr {($blockage_llx - $row_llx)/$site_x}]
                    
                    set x_tmp [expr {$row_llx + ($row1_num_sites * $site_x) - [$incnr_master getWidth]}]
                    #set x_tmp [expr {round (1.0*$x_start/$site_x)*$site_x}]
                    set x_start [expr { int($x_tmp) }]

                    set x_end [expr $blockage_urx]
                    set x_tmp [expr {ceil (1.0*$x_end/$site_x)*$site_x}]
                    set x_end [expr { int($x_tmp) }]

                    set inst1_name "PHY_${cnt}"
                    if {$x_start < $row_llx} {
                        set x_start $row_llx
                    } else {
                        # Insert cell at northwest corner
                        set inst1 [odb::dbInst_create $block $incnr_master $inst1_name]
                        if { $ori == "R0" } {
                            set cell_orient "MY"
                        } else {
                            set cell_orient "R180"
                        }
                        $inst1 setOrient $cell_orient
                        $inst1 setLocation $x_start $row_lly
                        $inst1 setPlacementStatus LOCKED
                    
                        incr cnt
                        incr blkgs_cnt
                    }

                    if {(($x_end + $endcapwidth) < $corebox_urx)} {
                        if {($x_end + $endcapwidth) <= $row_urx } {
                            # Insert cell at northeast corner
                            set inst2_name "PHY_${cnt}"
                            set inst2 [odb::dbInst_create $block $incnr_master $inst2_name]
                            $inst2 setOrient $ori
                            $inst2 setLocation $x_end $row_lly
                            $inst2 setPlacementStatus LOCKED

                            incr cnt
                            incr blkgs_cnt
                        } else {
                            set x_end [expr $row_urx - $endcapwidth]
                        }
                    } else {
                        set x_end [expr $corebox_urx - $endcapwidth]
                    }

                    #Insert remaining cells
                    set x_tb2 $x_start
                    for {set x [expr $x_start+$endcapwidth]} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                        set inst3_name "PHY_${cnt}"
                        set tie_diff [expr $x_end-($x+$tbtiewidth)]
                        set sites_gap [expr $tie_diff/$site_x]
                        if {$sites_gap <= 1} {
                            continue
                        }
                        set inst3 [odb::dbInst_create $block $tbtie_master $inst3_name]
                        $inst3 setOrient $ori
                        $inst3 setLocation $x $row_lly
                        $inst3 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                        set x_tb2 [expr $x+$tbtiewidth]
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

                    for {} {$x_tb2 < $x_end} {set x_tb2 [expr $x_tb2+(2*$site_x)]} {
                        set inst5_name "PHY_${cnt}"
                        set inst5 [odb::dbInst_create $block $tb2_master $inst5_name]
                        $inst5 setOrient $ori
                        $inst5 setLocation $x_tb2 $row_lly
                        $inst5 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                    }
                } elseif {($row_ury <= $blockage_lly)} {
                    # If row is at bottom of macro
                    if { $ori == "MX" } {
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

                    set cell_orient $ori

                    # Insert incnr cells
                    set row1_num_sites [expr {($blockage_llx - $row_llx)/$site_x}]
                    
                    set x_tmp [expr {$row_llx + ($row1_num_sites * $site_x) - [$incnr_master getWidth]}]
                    #set x_tmp [expr {round (1.0*$x_start/$site_x)*$site_x}]
                    set x_start [expr { int($x_tmp) }]
                    
                    set x_end [expr $blockage_urx]
                    set x_tmp [expr {ceil (1.0*$x_end/$site_x)*$site_x}]
                    set x_end [expr { int($x_tmp) }]

                    set inst1_name "PHY_${cnt}"
                    if {$x_start < $row_llx} {
                        set x_start $row_llx
                    } else {
                        # Insert cell at southwest corner
                        set inst1 [odb::dbInst_create $block $incnr_master $inst1_name]
                        if { $ori == "R0"} {
                            set cell_orient "MY"
                        } else {
                            set cell_orient "R180"
                        }
                        $inst1 setOrient $cell_orient
                        $inst1 setLocation $x_start $row_lly
                        $inst1 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                    }

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
                    set x_tb2 $x_start
                    for {set x [expr $x_start+$endcapwidth]} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
                        set inst3_name "PHY_${cnt}"
                        set tie_diff [expr $x_end-($x+$tbtiewidth)]
                        set sites_gap [expr $tie_diff/$site_x]
                        if {$sites_gap <= 1} {
                            continue
                        }
                        set inst3 [odb::dbInst_create $block $tbtie_master $inst3_name]
                        $inst3 setOrient $ori
                        $inst3 setLocation $x $row_lly
                        $inst3 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                        set x_tb2 [expr $x+$tbtiewidth]
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
                    
                    for {} {$x_tb2 < $x_end} {set x_tb2 [expr $x_tb2+(2*$site_x)]} {
                        set inst5_name "PHY_${cnt}"
                        set inst5 [odb::dbInst_create $block $tb2_master $inst5_name]
                        $inst5 setOrient $ori
                        $inst5 setLocation $x_tb2 $row_lly
                        $inst5 setPlacementStatus LOCKED

                        incr cnt
                        incr blkgs_cnt
                    }
                }
            }
        }
        utl::info TAP 7 "Cells inserted near blkgs: $blkgs_cnt"
    }

    #proc to detect even/odd
    proc even {row first_row_orient} {
        if {[$row getOrient] == $first_row_orient} {
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
        set rowBB [$row getBBox]
        set lly [$rowBB yMin]
        set ury [$rowBB yMax]
        
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
        set rowBB [$row getBBox]
        set row_llx [$rowBB xMin]
        set row_lly [$rowBB yMin]
        set row_urx [$rowBB xMax]
        set row_ury [$rowBB yMax]
        
        set row_height [expr $row_ury - $row_lly]

        set row_below_ury [expr $row_ury + $row_height]
        set row_above_lly [expr $row_lly - $row_height]

        foreach blockage $blockages {
            set blockage_llx [expr [$blockage xMin] - $halo_x]
            set blockage_lly [expr [$blockage yMin] - $halo_y]
            set blockage_urx [expr [$blockage xMax] + $halo_x]
            set blockage_ury [expr [$blockage yMax] + $halo_y]

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
        set blockage_llx [expr [$blockage xMin] - $halo_x]
        set blockage_lly [expr [$blockage yMin] - $halo_y]
        set blockage_urx [expr [$blockage xMax] + $halo_x]
        set blockage_ury [expr [$blockage yMax] + $halo_y]

        set rowBB [$row getBBox]
        set row_llx [$rowBB xMin]
        set row_lly [$rowBB yMin]
        set row_urx [$rowBB xMax]
        set row_ury [$rowBB yMax]

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
            set instBB [$inst getBBox]
            set inst_llx [$instBB xMin]
            set inst_lly [$instBB yMin]
            set inst_urx [$instBB xMax]
            set inst_ury [$instBB yMax]

            if {($inst_llx >= $ll_x) && ($inst_lly >= $ll_y) && \
                ($inst_urx <= $ur_x) && ($inst_ury <= $ur_y)} {
                lappend insts_in_area $inst
            }
        }

        return $insts_in_area
    }

    proc get_rows_top_bottom_macro {macro rows halo_x halo_y} {
        set macro_llx [expr [$macro xMin] - $halo_x]
        set macro_lly [expr [$macro yMin] - $halo_y]
        set macro_urx [expr [$macro xMax] + $halo_x]
        set macro_ury [expr [$macro yMax] + $halo_y]

        set top_bottom_rows ""

        foreach row $rows {
            set rowBB [$row getBBox]
            set row_llx [$rowBB xMin]
            set row_lly [$rowBB yMin]
            set row_urx [$rowBB xMax]
            set row_ury [$rowBB yMax]

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
        set rowBB [$row getBBox]
        set row_llx [$rowBB xMin]
        set row_lly [$rowBB yMin]
        set row_urx [$rowBB xMax]
        set row_ury [$rowBB yMax]

        set row_height [expr $row_ury - $row_lly]
        set row_below_ury [expr $row_ury + $row_height]
        set row_above_lly [expr $row_lly - $row_height]

        set top_bottom_macros ""

        foreach macro $macros {
            set macro_llx [expr [$macro xMin] - $halo_x]
            set macro_lly [expr [$macro yMin] - $halo_y]
            set macro_urx [expr [$macro xMax] + $halo_x]
            set macro_ury [expr [$macro yMax] + $halo_y]

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

    proc get_macros_overlapping_with_row {row macros halo_x halo_y} {
        set rowBB [$row getBBox]
        set row_llx [expr [$rowBB xMin] - $halo_x]
        set row_lly [$rowBB yMin]
        set row_urx [expr [$rowBB xMax] + $halo_x]
        set row_ury [$rowBB yMax]

        set overlap_macros ""

        set row_height [expr $row_ury - $row_lly]

        foreach macro $macros {
            set macro_llx [expr [$macro xMin] - $halo_x]
            set macro_lly [expr {[$macro yMin] - $halo_y - $row_height}]
            set macro_urx [expr [$macro xMax] + $halo_x]
            set macro_ury [expr {[$macro yMax] + $halo_y + $row_height}]

            set min_x [expr max($macro_llx, $row_llx)]
            set max_x [expr min($macro_urx, $row_urx)]
            
            set min_y [expr max($macro_lly, $row_lly)]
            set max_y [expr min($macro_ury, $row_ury)]

            set dx [expr $min_x - $max_x]
            set dy [expr $min_y - $max_y]

            if {[expr ($dx < 0) && ($dy < 0)]} {
                lappend overlap_macros $macro
            }
        }

        return $overlap_macros
    }

    proc in_blocked_region {x row row_blockages halo_x halo_y master_width endcapwidth} {
        if {([llength $row_blockages] > 0)} {
            foreach row_blockage $row_blockages {
                set row_blockage_llx [expr [$row_blockage xMin] - $halo_x]
                set row_blockage_urx [expr [$row_blockage xMax] + $halo_x]
                if {($x + $master_width) > ($row_blockage_llx - $endcapwidth) && \
                     $x < ($row_blockage_urx + $endcapwidth)} {
                    return 1
                }
            }
        }

        return 0
    }

    proc get_correct_llx {x row row_blockages halo_x halo_y master_width endcapwidth site_width add_boundary_cell} {
        set min_width [expr 2*$site_width]
        set urx [[$row getBBox] xMax]
        set end_llx [expr $urx - $endcapwidth] 
        set tap_urx [expr $x + $master_width]
        set increase -1

        if { !$add_boundary_cell } {
            return $x
        }

        if { ([llength $row_blockages] > 0) } {
            foreach row_blockage $row_blockages {
                set blockage_llx [expr [$row_blockage xMin] - $halo_x]
                set blockage_urx [expr [$row_blockage xMax] + $halo_x]

                set dist_llx [expr {$blockage_llx - $tap_urx}]
                set dist_urx [expr {$x - $blockage_urx}]

                if {$dist_llx == 0 || $dist_urx == 0} {
                    continue
                }

                set min_x [expr {$blockage_urx + $endcapwidth + $min_width}]
                set max_x [expr {$blockage_llx - $endcapwidth - $min_width - $master_width}]

                if { $x > $blockage_llx && $x < $min_x } {
                    set increase 1
                    set end_llx $min_x
                }

                if { $x < $blockage_urx && $x > $max_x } {
                    set increase 0
                    set end_llx $max_x
                }
            }
        }

        if { $increase == 1 } {
            while { $x < $end_llx } {
                set x [expr $x + $site_width]
            }
        } elseif { $increase == 0 } {
            while { $x > $end_llx } {
                set x [expr $x - $site_width]
            }
        }

        return $x
    }

    proc get_new_x {x row row_blockages halo_x halo_y master_width endcapwidth site_width} {
        set blocked_region 0
        set new_x $x

        if {([llength $row_blockages] > 0)} {
            foreach row_blockage $row_blockages {
                set row_blockage_llx [expr [$row_blockage xMin] - $halo_x]
                set row_blockage_urx [expr [$row_blockage xMax] + $halo_x]
                # if tapcell is only partially blocked at the right side of the blocked region
                if { ($x + $master_width) > ($row_blockage_urx + $endcapwidth) && \
                      $x < ($row_blockage_urx + $endcapwidth) } {
                    # move this tapcell to avoid the blocked region
                    set tmp_x [expr {ceil ((1.0*$row_blockage_urx + $endcapwidth)/$site_width)*$site_width}]
                    set tmp_x [expr { int($tmp_x) }]
                    set new_x $tmp_x 
                }
            }
        }

        return $new_x
    }

    proc find_blockages {db} {
        set block [[$db getChip] getBlock]
        set blockages {}

        foreach inst [$block getInsts] {
            if { [$inst isBlock] } {
                if { ![$inst isPlaced] } {
                    utl::warn 20 "Macro [$inst getName] is not placed"
                    continue
                }
                lappend blockages [$inst getBBox]
            }
        }

        return $blockages
    }

    proc get_macros_top_bottom_rows {db blockages halo_x halo_y} {
        set block [[$db getChip] getBlock]
        set rows_top_bottom_blockages {}
        foreach row [$block getRows] {
            set top_bottom_blockages [get_macros_top_bottom_row $row $blockages $halo_x $halo_y]
            lappend rows_top_bottom_blockages $top_bottom_blockages
        }

        return $rows_top_bottom_blockages
    }

    proc get_macros_overlapping_rows {db blockages halo_x halo_y} {
        set block [[$db getChip] getBlock]
        set rows_overlapping_blockages {}
        foreach row [$block getRows] {
            set overlapping_blockages [get_macros_overlapping_with_row $row $blockages $halo_x $halo_y]
            lappend rows_overlapping_blockages $overlapping_blockages
        }

        return $rows_overlapping_blockages
    }
}
