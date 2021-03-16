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
    utl::warn TAP 14 "endcap_cpp option is deprecated."
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
    utl::warn TAP 14 "tbtie_cpp option is deprecated."
  }

  set add_boundary_cell [info exists flags(-add_boundary_cell)]
  if {[info exists flags(-no_cell_at_top_bottom)]} {
    utl::warn TAP 14 "no_cell_at_top_bottom option is deprecated."
  }

  set db [ord::get_db]

  set halo_y [ord::microns_to_dbu $halo_y]
  set halo_x [ord::microns_to_dbu $halo_x]

  set endcap_master [$db findMaster $endcap_master]
  if { $endcap_master == "NULL" } {
    utl::error TAP 10 "Master $endcap_master not found."
  }

  tap::cut_rows $db $endcap_master [tap::find_blockages $db] $halo_x $halo_y

  set rows [tap::organize_rows $db]

  set cnrcap_masters {}
  lappend cnrcap_masters $cnrcap_nwin_master $cnrcap_nwout_master

  set tap::phy_idx 0
  set tap::filled_sites []
  tap::insert_endcaps $db $rows $endcap_master $cnrcap_masters

  if {$add_boundary_cell} {
    set tap_nw_masters {}
    lappend tap_nw_masters $tap_nwintie_master $tap_nwin2_master $tap_nwin3_master \
    $tap_nwouttie_master $tap_nwout2_master $tap_nwout3_master

    set tap_macro_masters {}
    lappend tap_macro_masters $incnrcap_nwin_master $tap_nwin2_master $tap_nwin3_master \
    $tap_nwintie_master $incnrcap_nwout_master $tap_nwout2_master \
    $tap_nwout3_master $tap_nwouttie_master

    tap::insert_at_top_bottom $db $rows $tap_nw_masters [$db findMaster $cnrcap_nwin_master]
    tap::insert_around_macros $db $rows $tap_macro_masters [$db findMaster $cnrcap_nwin_master]
  }

  tap::insert_tapcells $db $rows $tapcell_master $dist

  set tap::filled_sites []
}

namespace eval tap {
variable phy_idx
variable filled_sites

proc cut_rows {db endcap_master blockages halo_x halo_y} {
  set block [[$db getChip] getBlock]

  set rows_count [llength [$block getRows]]
  set block_count [llength $blockages]

  set end_width [$endcap_master getWidth]
  set min_row_width [expr 2*$end_width]

  # Gather rows needing to be cut up front
  set blocked_rows []
  set row_blockages [dict create]
  foreach blockage $blockages {
    foreach row [$block getRows] {
      if {[overlaps $blockage $row $halo_x $halo_y]} {
        set row_name [$row getName]
        if {![dict exists $row_blockages $row_name]} {
          lappend blocked_rows $row
        }
        dict lappend row_blockages $row_name [$blockage getBBox]
      }
    }
  }
  
  foreach row $blocked_rows {
    set row_name [$row getName]
    set row_bb [$row getBBox]

    set row_site [$row getSite]
    set site_width [$row_site getWidth]
    set orient [$row getOrient]
    set direction [$row getDirection]

    set start_origin_x [$row_bb xMin]
    set start_origin_y [$row_bb yMin]

    set curr_min_row_width [expr $min_row_width + 2*$site_width]

    set row_blockage_bboxs [dict get $row_blockages $row_name]
    set row_blockage_xs []
    foreach row_blockage_bbox [dict get $row_blockages $row_name] {
      lappend row_blockage_xs "[$row_blockage_bbox xMin] [$row_blockage_bbox xMax]"
    }
    set row_blockage_xs [lsort -integer -index 0 $row_blockage_xs]

    set row_sub_idx 1
    foreach blockage $row_blockage_xs {
      lassign $blockage blockage_x0 blockage_x1
      # ensure rows are an integer length of sitewidth
      set new_row_end_x [make_site_loc [expr $blockage_x0 - $halo_x] $site_width -1 $start_origin_x]
      build_row $block "${row_name}_$row_sub_idx" $row_site $start_origin_x $new_row_end_x $start_origin_y $orient $direction $curr_min_row_width
      incr row_sub_idx
      
      set start_origin_x [make_site_loc [expr $blockage_x1 + $halo_x] $site_width 1 $start_origin_x]
    }

    # Make last row
    build_row $block "${row_name}_$row_sub_idx" $row_site $start_origin_x [$row_bb xMax] $start_origin_y $orient $direction $curr_min_row_width
    
    # Remove current row
    odb::dbRow_destroy $row
  }

  set cut_rows_count [expr [llength [$block getRows]]-$rows_count]
  utl::info TAP 1 "Macro blocks found: $block_count"
  utl::info TAP 2 "Original rows: $rows_count"
  utl::info TAP 3 "Cut rows: $cut_rows_count"
}

proc insert_endcaps {db rows endcap_master cnrcap_masters} {
  variable phy_idx
  set start_phy_idx $phy_idx
  set block [[$db getChip] getBlock]

  set endcapwidth [$endcap_master getWidth]

  if {[llength $cnrcap_masters] == 2} {
    lassign $cnrcap_masters cnrcap_nwin_master cnrcap_nwout_master
    
    if {$cnrcap_nwin_master == "INVALID" || $cnrcap_nwout_master == "INVALID"} {
      set do_corners 0
    } else {
      set do_corners 1

      set cnrcap_nwin_master_db [$db findMaster $cnrcap_nwin_master]
      if { $cnrcap_nwin_master_db == "NULL" } {
        utl::error TAP 12 "Master $cnrcap_nwin_master not found."
      }
      set cnrcap_nwout_master_db [$db findMaster $cnrcap_nwout_master]

      if { $cnrcap_nwout_master_db == "NULL" } {
        utl::error TAP 13 "Master $cnrcap_nwout_master not found."
      }

      lassign [get_min_max_x $rows] min_x max_x
    }
  } else {
    set do_corners 0
  }

  set bottom_row 0
  set top_row [expr [llength $rows]-1]

  for {set cur_row $bottom_row} {$cur_row <= $top_row} {incr cur_row} {
    foreach subrow [lindex $rows $cur_row] {
      set row_bb [$subrow getBBox]
      set row_ori [$subrow getOrient]

      set llx [$row_bb xMin]
      set lly [$row_bb yMin]
      set urx [$row_bb xMax]
      set ury [$row_bb yMax]

      if {$do_corners} {
        if { $cur_row == $top_row } {
          set masterl [pick_corner_master 1 $row_ori $cnrcap_nwin_master_db $cnrcap_nwout_master_db $endcap_master]
          set masterr $masterl
        } elseif { $cur_row == $bottom_row } {
          set masterl [pick_corner_master -1 $row_ori $cnrcap_nwin_master_db $cnrcap_nwout_master_db $endcap_master]
          set masterr $masterl
        } else {
          set rows_above [lindex $rows [expr $cur_row + 1]]
          set rows_below [lindex $rows [expr $cur_row - 1]]
          set masterl [pick_corner_master [is_x_corner $llx $rows_above $rows_below] $row_ori $cnrcap_nwin_master_db $cnrcap_nwout_master_db $endcap_master]
          set masterr [pick_corner_master [is_x_corner $urx $rows_above $rows_below] $row_ori $cnrcap_nwin_master_db $cnrcap_nwout_master_db $endcap_master]
        }
      } else {
        set masterl $endcap_master
        set masterr $masterl
      }

      if {[$masterl getWidth] > [expr $urx - $llx]} {
        continue
      }

      build_cell $block $masterl $row_ori $llx $lly

      set master_x [$masterr getWidth]
      set master_y [$masterr getHeight]

      set loc_2_x [expr $urx - $master_x]
      set loc_2_y [expr $ury - $master_y]
      if {$llx == $loc_2_x && $lly == $loc_2_y} {
        utl::warn TAP 9 "row [$subrow getName] have enough space for only one endcap."
        continue
      }

      set right_ori $row_ori
      if { $row_ori == "MX" } {
        set right_ori "R180"
      } elseif { $row_ori == "R0"} {
        set right_ori "MY"
      }
      build_cell $block $masterr $right_ori $loc_2_x $loc_2_y
    }
  }

  set endcap_count [expr $phy_idx - $start_phy_idx]
  utl::info TAP 4 "Endcaps inserted: $endcap_count"
  return $endcap_count
}

proc is_x_in_row {x subrow} {
  foreach row $subrow {
    set row_bb [$row getBBox]
    if {$x >= [$row_bb xMin] && $x <= [$row_bb xMax]} {
      return 1
    }
  }
  return 0
}

proc is_x_corner {x rows_above rows_below} {
  set in_above [is_x_in_row $x $rows_above]
  set in_below [is_x_in_row $x $rows_below]

  if {$in_above && !$in_below} {
    return -1
  }
  if {!$in_above && $in_below} {
    return 1
  }
  return 0
}

proc pick_corner_master {top_bottom ori cnrcap_nwin_master cnrcap_nwout_master endcap_master} {
  if { $top_bottom == 1 } {
    if { $ori == "MX" } {
      return $cnrcap_nwin_master
    } else {
      return $cnrcap_nwout_master
    }
  } elseif { $top_bottom == -1 } {
    if { $ori == "R0" } {
      return $cnrcap_nwin_master
    } else {
      return $cnrcap_nwout_master
    }
  } else {
    return $endcap_master
  }
}

proc insert_tapcells {db rows tapcell_master dist} {
  variable filled_sites
  variable phy_idx
  set start_phy_idx $phy_idx
  set block [[$db getChip] getBlock]

  set master [$db findMaster $tapcell_master]
  if { $master == "NULL" } {
    utl::error TAP 16 "Master $tapcell_master not found."
  }
  set tap_width [$master getWidth]

  set dist1 [ord::microns_to_dbu $dist]
  set dist2 [ord::microns_to_dbu [expr 2*$dist]]

  set row_fills [dict create]
  foreach placement $filled_sites {
    lassign $placement y x_start x_end
    dict lappend row_fills $y "$x_start $x_end"
  }
  foreach y [dict keys $row_fills] {
    set merged_placements []
    set prev_x []
    foreach xs [lsort -integer -index 0 [dict get $row_fills $y]] {
      if {[llength $prev_x] == 0} {
        set prev_x $xs
      } else {
        if {[lindex $xs 0] == [lindex $prev_x 1]} {
          lset prev_x 1 [lindex $xs 1]
        } else {
          lappend merged_placements $prev_x
          set prev_x $xs
        }
      }
    }
    if {[llength $prev_x] != 0} {
      lappend merged_placements $prev_x
    }
    if {[llength $prev_x] == 0} {
      lappend merged_placements $xs
    }
    dict set row_fills $y $merged_placements
  }

  set rows_with_macros [dict create]
  foreach {key macro_rows} [get_macro_outlines $rows] {
    foreach {bot_row top_row} $macro_rows {
      incr bot_row -1
      incr top_row

      dict set rows_with_macros $bot_row 0
      dict set rows_with_macros $top_row 0
    }
  }
  set rows_with_macros [lsort -integer [dict keys $rows_with_macros]]

  for {set row_idx 0} {$row_idx < [llength $rows]} {incr row_idx} {
    set subrows [lindex $rows $row_idx]
    set row_y [[[lindex $subrows 0] getBBox] yMin]
    if {[dict exists $row_fills $row_y]} {
      set row_fill_check [dict get $row_fills $row_y]
    } else {
      set row_fill_check []
    }

    set gaps_above_below 0
    if {[lindex $rows_with_macros 0] == $row_idx} {
      set gaps_above_below 1
      set rows_with_macros [lrange $rows_with_macros 1 end]
    }

    foreach row $subrows {
      set site_x [[$row getSite] getWidth]
      set row_bb [$row getBBox]
      set llx [$row_bb xMin]
      set lly [$row_bb yMin]
      set urx [$row_bb xMax]
      set ury [$row_bb yMax]
      set ori [$row getOrient]

      set offset 0
      set pitch -1

      if {[expr $row_idx % 2] == 0} {
        set offset $dist1
      } else {
        set offset $dist2
      }

      if {$row_idx == 0 || $row_idx == [expr [llength $rows]-1] || $gaps_above_below} {
        set pitch $dist1
        set offset $dist1
      } else {
        set pitch $dist2
      }

      for {set x [expr $llx+$offset]} {$x < $urx} {set x [expr $x+$pitch]} {
        set x [make_site_loc $x $site_x -1 $llx]

        # check if site is filled
        set overlap [check_if_filled $x $tap_width $ori $row_fill_check]
        if {$overlap == 0} {
          build_cell $block $master $ori $x $lly "TAP_"
        } else {
          lassign $overlap new_x_left new_x_right

          if {$new_x_left >= $llx && [check_if_filled $new_x_left $tap_width $ori $row_fill_check] == 0} {
            # bump cell to the left
            build_cell $block $master $ori $new_x_left $lly "TAP_"
          } elseif {$new_x_right <= $urx && [check_if_filled $new_x_right $tap_width $ori $row_fill_check] == 0} {
            # bump cell to the left
            build_cell $block $master $ori $new_x_right $lly "TAP_"
          } else {
            # do nothing because legal site not possible
          }
        }
      }
    }
  }
  set tapcell_count [expr $phy_idx - $start_phy_idx]
  utl::info TAP 5 "Tapcells inserted: $tapcell_count"
  return $tapcell_count
}

proc check_if_filled {x width orient row_insts} {
  if {$orient == "MY" || $orient == "R180"} {
    set x_start [expr $x-$width]
    set x_end $x
  } else {
    set x_start $x
    set x_end [expr $x+$width]
  }

  foreach placement $row_insts {
    if {$x_end >= [lindex $placement 0] && $x_start <= [lindex $placement 1]} {
      set left_x [expr [lindex $placement 0] - $width]
      set right_x [lindex $placement 1]
      return "$left_x $right_x"
    }
  }
  return 0
}

proc insert_at_top_bottom {db rows masters endcap_master} {
  variable phy_idx
  variable row_info
  set start_phy_idx $phy_idx
  set block [[$db getChip] getBlock]

  if {[llength $masters] == 6} {
    lassign $masters tap_nwintie_master_name tap_nwin2_master_name tap_nwin3_master_name \
    tap_nwouttie_master_name tap_nwout2_master_name tap_nwout3_master_name
  }

  set tap_nwintie_master [$db findMaster $tap_nwintie_master_name]
  if { $tap_nwintie_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwintie_master_name not found."
  }
  set tap_nwin2_master [$db findMaster $tap_nwin2_master_name]
  if { $tap_nwin2_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwin2_master_name not found."
  }
  set tap_nwin3_master [$db findMaster $tap_nwin3_master_name]
  if { $tap_nwin3_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwin3_master_name not found."
  }
  set tap_nwouttie_master [$db findMaster $tap_nwouttie_master_name]
  if { $tap_nwouttie_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwouttie_master_name not found."
  }
  set tap_nwout2_master [$db findMaster $tap_nwout2_master_name]
  if { $tap_nwout2_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwout2_master_name not found."
  }
  set tap_nwout3_master [$db findMaster $tap_nwout3_master_name]
  if { $tap_nwout3_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwout3_master_name not found."
  }

  set tbtiewidth [$tap_nwintie_master getWidth]
  set endcapwidth [$endcap_master getWidth]

  set new_rows []
  # grab bottom
  lappend new_rows [lindex $rows 0]
  # grab top
  lappend new_rows [lindex $rows end]
  
  for {set cur_row 0} {$cur_row < [llength $new_rows]} {incr cur_row} {
    foreach subrow [lindex $new_rows $cur_row] {
      set site_x [[$subrow getSite] getWidth]

      set row_bb [$subrow getBBox]
      set llx [$row_bb xMin]
      set lly [$row_bb yMin]
      set urx [$row_bb xMax]
      set ury [$row_bb yMax]

      set ori [$subrow getOrient]

      set x_start [expr $llx+$endcapwidth]
      set x_end [expr $urx-$endcapwidth]

      insert_at_top_bottom_helper $block $cur_row $ori $x_start $x_end $lly $tap_nwintie_master $tap_nwin2_master $tap_nwin3_master $tap_nwouttie_master $tap_nwout2_master $tap_nwout3_master
    }
  }
  set topbottom_cnt [expr $phy_idx - $start_phy_idx]
  utl::info TAP 6 "Top/bottom cells inserted: $topbottom_cnt"
  return $topbottom_cnt
}

proc insert_at_top_bottom_helper {block top_bottom ori x_start x_end lly tap_nwintie_master tap_nwin2_master tap_nwin3_master tap_nwouttie_master tap_nwout2_master tap_nwout3_master} {
  if {$top_bottom == 1} {
    # top
    if { $ori == "MX" } {
      set master $tap_nwintie_master
      set tb2_master $tap_nwin2_master
      set tb3_master $tap_nwin3_master
    } else {
      set master $tap_nwouttie_master
      set tb2_master $tap_nwout2_master
      set tb3_master $tap_nwout3_master
    }
  } else {
    # bottom
    if { $ori == "R0" } {
      set master $tap_nwintie_master
      set tb2_master $tap_nwin2_master
      set tb3_master $tap_nwin3_master
    } else {
      set master $tap_nwouttie_master
      set tb2_master $tap_nwout2_master
      set tb3_master $tap_nwout3_master
    }
  }

  set tbtiewidth [$master getWidth]
  #insert tb tie
  for {set x $x_start} {$x+$tbtiewidth < $x_end} {set x [expr $x+$tbtiewidth]} {
    build_cell $block $master $ori $x $lly
  }

  # fill remaining sites
  set tap_nwout3_master_width [$tap_nwout3_master getWidth]
  set tap_nwout2_master_width [$tap_nwout2_master getWidth]
  
  set x_end_tb3_test [expr ($x_end - $x) % $tap_nwout3_master_width]
  if {$x_end_tb3_test == 0} {
    set x_end_tb3 $x_end
  } elseif {$x_end_tb3_test == $tap_nwout2_master_width} {
    set x_end_tb3 [expr $x_end - $tap_nwout2_master_width]
  } else {
    set x_end_tb3 [expr $x_end - 2*$tap_nwout2_master_width]
  }

  # fill with 3s
  for {} {$x < $x_end_tb3} {set x [expr $x+$tap_nwout3_master_width]} {
    build_cell $block $tb3_master $ori $x $lly
  }
  
  # fill with 2s
  for {} {$x < $x_end} {set x [expr $x+$tap_nwout2_master_width]} {
    build_cell $block $tb2_master $ori $x $lly
  }
}

proc insert_around_macros {db rows masters corner_master} {
  variable phy_idx
  set start_phy_idx $phy_idx
  set block [[$db getChip] getBlock]

  if {[llength $masters] == 8} {
    lassign $masters incnrcap_nwin_master_name tap_nwin2_master_name tap_nwin3_master_name tap_nwintie_master_name \
    incnrcap_nwout_master_name tap_nwout2_master_name tap_nwout3_master_name tap_nwouttie_master_name
  }
  set tap_nwintie_master [$db findMaster $tap_nwintie_master_name]
  set tap_nwin2_master [$db findMaster $tap_nwin2_master_name]
  set tap_nwin3_master [$db findMaster $tap_nwin3_master_name]
  set tap_nwouttie_master [$db findMaster $tap_nwouttie_master_name]
  set tap_nwout2_master [$db findMaster $tap_nwout2_master_name]
  set tap_nwout3_master [$db findMaster $tap_nwout3_master_name]
  set incnrcap_nwin_master [$db findMaster $incnrcap_nwin_master_name]
  set incnrcap_nwout_master [$db findMaster $incnrcap_nwout_master_name]
  
  set tap_nwintie_master [$db findMaster $tap_nwintie_master_name]
  if { $tap_nwintie_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwintie_master_name not found."
  }
  set tap_nwin2_master [$db findMaster $tap_nwin2_master_name]
  if { $tap_nwin2_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwin2_master_name not found."
  }
  set tap_nwin3_master [$db findMaster $tap_nwin3_master_name]
  if { $tap_nwin3_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwin3_master_name not found."
  }
  set tap_nwouttie_master [$db findMaster $tap_nwouttie_master_name]
  if { $tap_nwouttie_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwouttie_master_name not found."
  }
  set tap_nwout2_master [$db findMaster $tap_nwout2_master_name]
  if { $tap_nwout2_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwout2_master_name not found."
  }
  set tap_nwout3_master [$db findMaster $tap_nwout3_master_name]
  if { $tap_nwout3_master == "NULL" } {
    utl::error TAP 16 "Master $tap_nwout3_master_name not found."
  }
  set incnrcap_nwin_master [$db findMaster $incnrcap_nwin_master_name]
  if { $incnrcap_nwin_master == "NULL" } {
    utl::error TAP 16 "Master $incnrcap_nwin_master_name not found."
  }
  set incnrcap_nwout_master [$db findMaster $incnrcap_nwout_master_name]
  if { $incnrcap_nwout_master == "NULL" } {
    utl::error TAP 16 "Master $incnrcap_nwout_master_name not found."
  }
  
  # find macro outlines
  set macro_outlines [get_macro_outlines $rows]
  
  set corner_cell_width [$corner_master getWidth]
  
  foreach {key outlines} $macro_outlines {
    lassign $key x_start x_end

    foreach {bot_row top_row} $outlines {
      # move to actual rows
      incr bot_row -1
      incr top_row
      
      set bot_row [lindex $rows $bot_row 0]
      set top_row [lindex $rows $top_row 0]
      
      set bot_row_ori [$bot_row getOrient]
      set top_row_ori [$top_row getOrient]
      
      set bot_row_y [[$bot_row getBBox] yMin]
      set top_row_y [[$top_row getBBox] yMin]
      
      set row_start $x_start
      set row_end $x_end
      if {$row_start == -1} {
        set row_start [[$bot_row getBBox] xMin]
        set row_start [expr $row_start + $corner_cell_width]
      }
      if {$row_end == -1} {
        set row_end [[[lindex $rows $bot_row end] getBBox] xMax]
        set row_end [expr $row_end - $corner_cell_width]
      }

      # do bottom row
      insert_at_top_bottom_helper $block 0 $bot_row_ori $row_start $row_end $bot_row_y $tap_nwintie_master $tap_nwin2_master $tap_nwin3_master $tap_nwouttie_master $tap_nwout2_master $tap_nwout3_master
      
      set row_start $x_start
      set row_end $x_end
      if {$row_start == -1} {
        set row_start [[$top_row getBBox] xMin]
        set row_start [expr $row_start + $corner_cell_width]
      }
      if {$row_end == -1} {
        set row_end [[[lindex $rows $top_row end] getBBox] xMax]
        set row_end [expr $row_end - $corner_cell_width]
      }
      # do top row
      insert_at_top_bottom_helper $block 1 $top_row_ori $row_start $row_end $top_row_y $tap_nwintie_master $tap_nwin2_master $tap_nwin3_master $tap_nwouttie_master $tap_nwout2_master $tap_nwout3_master
          
      # do corners
      # top row
      if { $top_row_ori == "R0" } {
        set incnr_master $incnrcap_nwin_master
        set west_ori "MY"
      } else {
        set incnr_master $incnrcap_nwout_master
        set west_ori "R180"
      }

      # NE corner
      build_cell $block $incnr_master $top_row_ori [expr $x_start - [$incnr_master getWidth]] $top_row_y
      # NW corner
      build_cell $block $incnr_master $west_ori $x_end $top_row_y
      
      # bottom row
      if { $bot_row_ori == "MX" } {
        set incnr_master $incnrcap_nwin_master
        set west_ori "R180"
      } else {
        set incnr_master $incnrcap_nwout_master
        set west_ori "MY"
      }

      # SE corner
      build_cell $block $incnr_master $top_row_ori [expr $x_start - [$incnr_master getWidth]] $bot_row_y
      # SW corner
      build_cell $block $incnr_master $west_ori $x_end $bot_row_y
    }
  }
  set blkgs_cnt [expr $phy_idx - $start_phy_idx]
  utl::info TAP 7 "Cells inserted near blkgs: $blkgs_cnt"
  return $blkgs_cnt
}

proc get_min_max_x {rows} {
  set min_x -1
  set max_x -1
  foreach subrow $rows {
    foreach row $subrow {
      set row_bb [$row getBBox]
      set new_min_x [$row_bb xMin]
      set new_max_x [$row_bb xMax]
      if {$min_x == -1} {
        set min_x $new_min_x
      } elseif {$min_x > $new_min_x} {
        set min_x $new_min_x
      }
      if {$max_x == -1} {
        set max_x $new_max_x
      } elseif {$max_x < $new_max_x} {
        set max_x $new_max_x
      }
    }
  }

  return "$min_x $max_x"
}

proc get_macro_outlines {rows} {
  set macro_outlines [dict create]

  lassign [get_min_max_x $rows] min_x max_x

  for {set cur_row 0} {$cur_row < [llength $rows]} {incr cur_row} {
    set subrows [lindex $rows $cur_row]
    set subrow_count [llength $subrows]
    if {$subrow_count == 1} {
      set sample_row [[lindex $subrows 0] getBBox]
      set row_min_x [$sample_row xMin]
      set row_max_x [$sample_row xMax]
      if {$row_min_x != $min_x || $row_max_x != $max_x} {
        set leftrow_end -1
        set rightrow_start -1
        if {$row_min_x != $min_x} {
          set rightrow_start $row_min_x
        }
        if {$row_max_x != $max_x} {
          set leftrow_end $row_max_x
        }
        dict lappend macro_outlines "$leftrow_end $rightrow_start" $cur_row
      }
      continue
    }

    set firstrow_bb [[lindex $subrows 0] getBBox]
    if {[$firstrow_bb xMin] > $min_x} {
      dict lappend macro_outlines "-1 [$firstrow_bb xMin]" $cur_row
    }

    for {set cur_subrow 0} {$cur_subrow < $subrow_count-1} {incr cur_subrow} {
      set leftrow [lindex $subrows $cur_subrow]
      set rightrow [lindex $subrows [expr $cur_subrow+1]]

      set leftrow_end [[$leftrow getBBox] xMax]
      set rightrow_start [[$rightrow getBBox] xMin]

      dict lappend macro_outlines "$leftrow_end $rightrow_start" $cur_row
    }

    set lastrow_bb [[lindex $subrows end] getBBox]
    if {[$lastrow_bb xMax] < $max_x} {
      dict lappend macro_outlines "[$lastrow_bb xMax] -1" $cur_row
    }
  }

  set macro_outlines_array []
  foreach key [dict keys $macro_outlines] {
    set all_rows [dict get $macro_outlines $key]
    set new_rows []

    lappend new_rows [lindex $all_rows 0]
    for {set i 1} {$i < [llength $all_rows]-1} {incr i} {
      if {[expr [lindex $all_rows $i]+1] == [lindex $all_rows [expr $i+1]]} {
        continue
      }
      lappend new_rows [lindex $all_rows $i]
      lappend new_rows [lindex $all_rows [expr $i+1]]
    }
    lappend new_rows [lindex $all_rows end]

    lappend macro_outlines_array $key $new_rows
  }

  return $macro_outlines_array
}

#proc to detect if blockage overlaps with row
proc overlaps {blockage row halo_x halo_y} {
  set blockageBB [$blockage getBBox]
  set rowBB [$row getBBox]

  # check if Y has overlap first since rows are long and skinny 
  set blockage_lly [expr [$blockageBB yMin] - $halo_y]
  set blockage_ury [expr [$blockageBB yMax] + $halo_y]
  set row_lly [$rowBB yMin]
  set row_ury [$rowBB yMax]
  
  if {$blockage_lly >= $row_ury || $row_lly >= $blockage_ury} {
    return 0
  }

  set blockage_llx [expr [$blockageBB xMin] - $halo_x]
  set blockage_urx [expr [$blockageBB xMax] + $halo_x]
  set row_llx [$rowBB xMin]
  set row_urx [$rowBB xMax]

  if {$blockage_llx >= $row_urx || $row_llx >= $blockage_urx} {
    return 0
  }
  
  return 1
}

proc find_blockages {db} {
  set block [[$db getChip] getBlock]
  set blockages {}

  foreach inst [[[$db getChip] getBlock] getInsts] {
    if { [$inst isBlock] } {
      if { ![$inst isPlaced] } {
        utl::warn 20 "Macro [$inst getName] is not placed"
        continue
      }
      lappend blockages $inst
    }
  }

  return $blockages
}

proc make_site_loc {x site_x {dirc 1} {offset 0}} {
  set site_int [expr double($x - $offset) / $site_x]
  if {$dirc == 1} {
    set site_int [expr ceil( $site_int )]
  } else {
    set site_int [expr floor( $site_int )]
  }
  return [expr int($site_int * $site_x + $offset)]
}

proc build_cell {block master orientation x y {prefix "PHY_"}} {
  variable phy_idx
  variable filled_sites

  if {$x == -1 || $y == -1} {
    return
  }
  set name "${prefix}${phy_idx}"
  set inst [odb::dbInst_create $block $master $name]
  if {$inst == "NULL"} {
    return
  }
  $inst setOrient $orientation
  $inst setLocation $x $y
  $inst setPlacementStatus LOCKED

  set inst_bb [$inst getBBox]

  lappend filled_sites "[$inst_bb yMin] [$inst_bb xMin] [$inst_bb xMax]"

  incr phy_idx
}

proc build_row {block name site start_x end_x y orient direction min_row_width} {
  set site_width [$site getWidth]

  set new_row_num_sites [expr ($end_x - $start_x)/$site_width]
  set new_row_width [expr $new_row_num_sites*$site_width]

  if {$new_row_num_sites > 0 && $new_row_width >= $min_row_width} {
    odb::dbRow_create $block $name $site $start_x $y $orient $direction $new_row_num_sites $site_width
  }
}

proc organize_rows {db} {
  set rows_dict [dict create]
  foreach row [[[$db getChip] getBlock] getRows] {
    dict lappend rows_dict [[$row getBBox] yMin] $row
  }

  # organize rows bottom to top
  set rows []
  foreach key [lsort -integer [dict keys $rows_dict]] {
    set in_row_dict [dict create]
    foreach in_row [dict get $rows_dict $key] {
      dict lappend in_row_dict [[$in_row getBBox] xMin] $in_row
    }
    # organize sub rows left to right
    set in_row []
    foreach in_key [lsort -integer [dict keys $in_row_dict]] {
      lappend in_row [dict get $in_row_dict $in_key]
    }
    lappend rows $in_row
    unset in_row
  }
  unset rows_dict

  return $rows
}

# namespace end
}
