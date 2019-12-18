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

sta::define_cmd_args "run_tapcell" {[-output_file out_file] \
                                    [-tech tech] \
                                    [-tabcell_master tabcell_master] \
                                    [-endcap_master endcap_master] \
                                    [-endcap_cpp endcap_cpp] \
#when you set 25 (um), each row has 50um with checker-board pattern
                                    [-distance dist] \
                                    [-halo_macro halo_macro] \
}

#Pre-step: assumed that placement blockages inserted around macros
#You might or might not need this step
#createPlaceBlockage -allMacro -outerRingBySide {$halo_macro $halo_macro $halo_macro $halo_macro}

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
    proc overlaps {blockage, row} {
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

        if {{$dx < 0} && {$dy < 0}} {
            return 1
        } else {
            return 0
        }
    }
}

# Main function. It will run tapcell given the correct parameters
proc run_tapcell { args } {
    sta::parse_key_args "run_tapcell" args \
        keys {-output_file -tech -endcap_master -endcap_cpp -distance -halo_macro} flags {}

    if { [info exists keys(-output_file)] } {
        set out_file $keys(-output_file)
        puts $out_file
    } else {
        puts "WARNING: Default output guide name: out.guide"
    }

    if { [info exists keys(-tech)] } {
        set tech $keys(-tech)
    } else {
        puts "ERROR: Please, set the technology with '-tech' flag"
        puts "\t--Current options: Nangate45, gf14"
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

    if { [info exists keys(-halo_macro)] } {
        set halo_macro $keys(-halo_macro)
    }

    if { [string match $tech "Nangate45"] } {
        puts "Processing Nangate45"
        
        set db [::ord::get_db]
        set block [[$db getChip] getBlock]
        set rows [$block getRows]

        #Step 1: cut placement rows if there are overlaps between rows and placement blockages

        foreach blockage [$block getBlockages] {
            foreach row $rows {
                if {[tapcell::overlaps $blockage $rows]} {
                    # Create two new rows, avoiding overlap with blockage
                    set row_site [$row getSite]
                    set orient [$row getOrient]
                    set direction [$row getDirection]

                    set site_width [$row_site getWidth]

                    ## First new row: from left of previous row to the left boundary of blockage
                    set first_origin_x [[$row getBBox] xMin]
                    set first_origin_y [[$row getBBox] yMin]
                    set first_end_x [[$blockage getBBox] xMin]
                    set first_num_sites [expr {($first_end_x - $first_origin_x)/$site_width}]
                    
                    if {first_num_sites > 0} {
                        odb::dbRow_create $block $row_name $row_site $first_origin_x $first_origin_y $orient $direction $first_num_sites $site_width
                    }

                    ## Second new row: from right of previous row to the right boundary of blockage
                    set second_origin_x [[$blockage getBBox] xMax]
                    set second_origin_y [[$row getBBox] yMin]
                    set second_end_x [[$row getBBox] xMax]
                    set second_num_sites [expr {($second_end_x - $second_origin_x)/$site_width}]
                    
                    if {second_num_sites > 0} {
                        odb::dbRow_create $block $row_name $row_site $second_origin_x $second_origin_y $orient $direction $second_num_sites $site_width
                    }

                    # Remove current row
                    odb::dbRow_destroy $row
                }
            }
        }

        #Step 2: Insert Endcap at the left and right end of each row

        set cnt 0
        foreach row $rows {
            set site_x [[$row getSite] getWidth]
            set site_y [[$row getSite] getHeight]
            set llx [[$row getBBox] xMin]
            set lly [[$row getBBox] yMin]
            set urx [[$row getBBox] xMax]
            set ury [[$row getBBox] yMax]
            set loc_2_x [expr $urx - $site_x]
            set loc_2_y [expr $ury - $site_y]

            set loc_1 "$llx $lly"
            set loc_2 "$loc_2_x $loc_2_y"

            set ori [$row getOrient]

            #addInst -cell $endcap_master -inst "PHY_${cnt}" -physical -loc $loc_1 -ori $ori # TODO: change to OpenDB cmd
            incr cnt
            #addInst -cell $endcap_master -inst "PHY_${cnt}" -physical -loc $loc_2 -ori $ori # TODO: change to OpenDB cmd
            incr cnt
        }
        
        #Step 3: Insert tab
        set core_box_lly [[$block getBBox] yMin]

        foreach row $rows {
            set llx [[$row getBBox] xMin]
            set lly [[$row getBBox] yMin]
            set urx [[$row getBBox] xMax]
            set ury [[$row getBBox] yMax]

            set ori [$row getOrient]

            if {[tapcell::even $row]} {
                set offset $dist
            } else {
                set offset [expr $dist*2]
            }

            if {[tapcell::top_or_bottom $row]} {
                set pitch $dist
            } else {
                set pitch [expr $dist*2]
            }

            for {set x [expr $llx+$offset]} {$x < [expr $urx-$endcap_cpp*$site_x]} {set x [expr $x+$pitch]} {
                set loc "$x $lly"
                #addInst -cell $tabcell_master -inst "PHY_${cnt}" -physical -loc $loc -ori $ori # TODO: change to OpenDB cmd
                incr cnt
            }
        }
    } elseif { [string match $tech "gf14"] } {
        puts "WARNING: Currently, tapcell does not support gf14"
    } else {
        puts "ERROR: informed tech is not valid"
    }   
}