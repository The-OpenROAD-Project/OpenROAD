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

    #proc to detect top/bottom row
    proc top_or_bottom {row} {
        set db [::ord::get_db]
        set block [[$db getChip] getBlock]

        set lly [[$row getBBox] yMin]
        set ury [[$row getBBox] yMax]
        set core_box_lly [[$block getBBox] yMin]
        set core_box_ury [[$block getBBox] yMax]

        if {$lly == $core_box_lly} {
            return 1
        } elseif {$ury == $core_box_ury} {
            return 1
        } else {
            return 0
        }
    }

    #proc to detect if blockage overlaps with row
    proc overlaps {blockage row} {
        set blockage_llx [[$blockage getBBox] xMin]
        set blockage_lly [[$blockage getBBox] yMin]
        set blockage_urx [[$blockage getBBox] xMax]
        set blockage_ury [[$blockage getBBox] yMax]

        set row_llx [[$row getBBox] xMin]
        set row_lly [[$row getBBox] yMin]
        set row_urx [[$row getBBox] xMax]
        set row_ury [[$row getBBox] yMax]

        set min_x [expr min($blockage_llx, $row_llx)]
        set max_x [expr max($blockage_urx, $row_urx)]
        set min_y [expr min($blockage_lly, $row_lly)]
        set max_y [expr max($blockage_ury, $row_ury)]

        set dx [expr $max_x - $min_x]
        set dy [expr $max_y - $min_y]

        set overlap [expr ($dx < 0) && ($dy < 0)]

        return $overlap
    }
}

# Main function. It will run tapcell given the correct parameters
proc tapcell { args } {
    sta::parse_key_args "tapcell" args \
        keys {-tapcell_master -endcap_master -endcap_cpp -distance} flags {}

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
    }
        
    set db [::ord::get_db]
    set block [[$db getChip] getBlock]
    set libs [$db getLibs]
    set rows [$block getRows]

    #Step 1: cut placement rows if there are overlaps between rows and placement blockages

    foreach blockage [$block getInsts] {
        set inst_master [$blockage getMaster]
        if { [string match [$inst_master getType] "BLOCK"] } {
            foreach row $rows {
                if {[tapcell::overlaps $blockage $row]} {
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
                    set row1_end_x [[$blockage getBBox] xMin]
                    set row1_num_sites [expr {($row1_end_x - $row1_origin_x)/$site_width}]

                    if {$row1_num_sites > 0} {
                        odb::dbRow_create $block $row1_name $row_site $row1_origin_x $row1_origin_y $orient $direction $row1_num_sites $site_width
                    }

                    ## Second new row: from right of original  row to the right boundary of blockage
                    set blockage_x_max [[$blockage getBBox] xMax]

                    set row2_origin_x [expr {ceil (1.0*$blockage_x_max/$site_width)*$site_width}]
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
    }

    #Step 2: Insert Endcap at the left and right end of each row

    set cnt 0
    foreach row $rows {
        set master [$db findMaster $endcap_master]
        if { [string match [$master getConstName] $endcap_master] } {
            set master_x [$master getWidth]
            set master_y [$master getHeight]
            
            set llx [[$row getBBox] xMin]
            set lly [[$row getBBox] yMin]
            set urx [[$row getBBox] xMax]
            set ury [[$row getBBox] yMax]

            set loc_2_x [expr $urx - $master_x]
            set loc_2_y [expr $ury - $master_y]

            set ori [$row getOrient]

            set inst1_name "PHY_${cnt}"
            set inst1 [odb::dbInst_create $block $master $inst1_name]
            $inst1 setOrient $ori
            $inst1 setLocation $llx $lly
            $inst1 setPlacementStatus LOCKED

            incr cnt

            set inst2_name "PHY_${cnt}"
            set inst2 [odb::dbInst_create $block $master $inst2_name]
            $inst2 setOrient $ori
            $inst2 setLocation $loc_2_x $loc_2_y
            $inst2 setPlacementStatus LOCKED

            incr cnt
        } else {
            puts "ERROR Master $endcap_master not found"
            exit 1
        }
    }

    #Step 3: Insert tap

    foreach row $rows {
        set site_x [[$row getSite] getWidth]
        set llx [[$row getBBox] xMin]
        set lly [[$row getBBox] yMin]
        set urx [[$row getBBox] xMax]
        set ury [[$row getBBox] yMax]

        set ori [$row getOrient]

        foreach lib $libs {
            set lef_units [$lib getLefUnits]
        }

        if {[tapcell::even $row]} {
            set offset [expr $dist*$lef_units]
        } else {
            set offset [expr $dist*2*$lef_units]
        }

        if {[tapcell::top_or_bottom $row]} {
            set pitch [expr $dist*$lef_units]
        } else {
            set pitch [expr $dist*2*$lef_units]
        }

        for {set x [expr $llx+$offset]} {$x < [expr $urx-$endcap_cpp*$site_x]} {set x [expr $x+$pitch]} {
            set master [$db findMaster $tabcell_master]
            set inst_name "PHY_${cnt}"

            if { [string match [$master getConstName] $tabcell_master] } {
                set inst [odb::dbInst_create $block $master $inst_name]
                $inst setLocation $x $lly
                $inst setOrient $ori
                $inst setPlacementStatus LOCKED

                incr cnt
            } else {
                puts "ERROR Master $tabcell_master not found"
                exit 1
            }
        }
    }
}
