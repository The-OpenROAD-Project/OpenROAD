#BSD 3-Clause License
#
#Copyright (c) 2019, The Regents of the University of California
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
#2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#3. Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

sta::define_cmd_args "run_pdngen" {[-key1 key1] [-flag1] pos_arg1}

# Put helper functions in a separate namespace so they are not visible
# too users in the global namespace.
namespace eval pdngen {
    variable logical_viarules {}
    variable physical_viarules {}
    variable vias {}
    variable stripe_locs
    variable orig_stripe_locs
    variable layers {}
    
    #This file contains procedures that are used for PDN generation

    proc lmap {args} {
        set result {}
        set var [lindex $args 0]
        foreach item [lindex $args 1] {
            uplevel 1 "set $var $item"
            lappend result [uplevel 1 [lindex $args end]]
        }
        return $result
    }

    proc get_dir {layer_name} {
        variable metal_layers 
        variable metal_layers_dir 
        if {[regexp {.*_PIN_(hor|ver)} $layer_name - dir]} {
            return $dir
        }
        
        set idx [lsearch $metal_layers $layer_name]
        if {[lindex $metal_layers_dir $idx] == "HORIZONTAL"} {
            return "hor"
        } else {
            return "ver"
        }
    }
    
    proc get_rails_layer {} {
        variable default_grid_data
        
        return [dict get $default_grid_data rails]
    }

    proc init_via_tech {} {
        variable tech
        variable def_via_tech
        
        set def_via_tech {}
        foreach via_rule [$tech getViaGenerateRules] {
            set lower [$via_rule getViaLayerRule 0]
            set upper [$via_rule getViaLayerRule 1]
            set cut   [$via_rule getViaLayerRule 2]

	    dict set def_via_tech [$via_rule getName] [list \
							   lower [list layer [[$lower getLayer] getName] enclosure [$lower getEnclosure]] \
							   upper [list layer [[$upper getLayer] getName] enclosure [$upper getEnclosure]] \
							   cut   [list layer [[$cut getLayer] getName] spacing [$cut getSpacing] size [list [[$cut getRect] dx] [[$cut getRect] dy]]] \
							  ]
        }
    }
    
    proc select_viainfo {lower} {
        variable def_via_tech

        set layer_name $lower
        regexp {(.*)_PIN} $lower - layer_name
        
        return [dict filter $def_via_tech script {rule_name rule} {expr {[dict get $rule lower layer] == $layer_name}}]
    }
    
    proc set_layer_info {layer_info} {
        variable layers
        
        set layers $layer_info
    }
    
    proc get_layer_info {} {
        variable layers 
        
        return $layers
    }
    
    # Layers that have a widthtable will only support some width values, the widthtable defines the 
    # set of widths that are allowed, or any width greater than or equal to the last value in the
    # table
    proc get_adjusted_width {layer width} {
        set layers [get_layer_info]
        
        if {[dict exists $layers $layer] && [dict exists $layers $layer widthtable]} {
            set widthtable [dict get $layers $layer widthtable]
            if {[lsearch $widthtable $width] > 0} {
                return $width
            } elseif {$width > [lindex $widthtable end]} {
                return $width
            } else {
                foreach value $widthtable {
                    if {$value > $width} {
                        return $value
                    }
                }
            }
        }
        
        return $width
    }
    
    # Given the via rule expressed in via_info, what is the via with the largest cut area that we can make
    proc get_via_option {lower_dir rule_name via_info x y width height} {
        set cut_width  [lindex [dict get $via_info cut size] 0]
        set cut_height [lindex [dict get $via_info cut size] 1]

        # Adjust the width and height values to the next largest allowed value if necessary
        set lower_width  [get_adjusted_width [dict get $via_info lower layer] $width]
        set lower_height [get_adjusted_width [dict get $via_info lower layer] $height]
        set upper_width  [get_adjusted_width [dict get $via_info upper layer] $width]
        set upper_height [get_adjusted_width [dict get $via_info upper layer] $height]
        
        set lower_enclosure [lindex [dict get $via_info lower enclosure] 0]
        set max_lower_enclosure [lindex [dict get $via_info lower enclosure] 1]
        
        if {$max_lower_enclosure < $lower_enclosure} {
            set swap $lower_enclosure
            set lower_enclosure $max_lower_enclosure
            set max_lower_enclosure $swap
        }

        set upper_enclosure [lindex [dict get $via_info upper enclosure] 0]
        set max_upper_enclosure [lindex [dict get $via_info upper enclosure] 1]

        if {$max_upper_enclosure < $upper_enclosure} {
            set swap $upper_enclosure
            set upper_enclosure $max_upper_enclosure
            set max_upper_enclosure $swap
        }
        
        # What are the maximum number of rows and columns that we can fit in this space?
        set i 0
        set via_width_lower 0
        set via_width_upper 0
        while {$via_width_lower < $lower_width && $via_width_upper < $upper_width} {
            incr i
            set xcut_pitch [lindex [dict get $via_info cut spacing] 0]
            set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $lower_enclosure]
            set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $upper_enclosure]
        }
        set xcut_spacing [expr $xcut_pitch - $cut_width]
        set columns [expr $i - 1]

        set i 0
        set via_height_lower 0
        set via_height_upper 0
        while {$via_height_lower < $lower_height && $via_height_upper < $upper_height} {
            incr i
            set ycut_pitch [lindex [dict get $via_info cut spacing] 1]
            set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $lower_enclosure]
            set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $upper_enclosure]
        }
        set ycut_spacing [expr $ycut_pitch - $cut_height]
        set rows [expr $i - 1]

	set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
	set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]
	set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
	set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]

        # Adjust calculated via width values to ensure that an allowed size is generated
        set lower_size_max_enclosure [get_adjusted_width [dict get $via_info lower layer] [expr round(($cut_width   + $xcut_pitch * ($columns - 1) + $max_lower_enclosure * 2))]]
        set upper_size_max_enclosure [get_adjusted_width [dict get $via_info upper layer] [expr round(($cut_width   + $xcut_pitch * ($columns - 1) + $max_upper_enclosure * 2))]]
        
        set max_lower_enclosure [expr round(($lower_size_max_enclosure  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
        set max_upper_enclosure [expr round(($upper_size_max_enclosure  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]

        # Use the largest value of enclosure in the direction of the layer
        # Use the smallest value of enclosure perpendicular to direction of the layer
	if {$lower_dir == "hor"} {
            if {$lower_enc_height < $max_lower_enclosure} {
                set xBotEnc $max_lower_enclosure
                if {$lower_enc_width > $xBotEnc} {
                    set xBotEnc $lower_enc_width
                }
            } else {
                set xBotEnc $lower_enc_width
            }
            set yBotEnc $lower_enc_height
        } else {
            set xBotEnc $lower_enc_width
            if {$lower_enc_width < $max_lower_enclosure} {
                set yBotEnc $max_lower_enclosure
                if {$lower_enc_height > $yBotEnc} {
                    set yBotEnc $lower_enc_height
                }
            } else {
                set yBotEnc $lower_enc_height
            }
        }

        # Use the largest value of enclosure in the direction of the layer
        # Use the smallest value of enclosure perpendicular to direction of the layer
	if {[get_dir [dict get $via_info upper layer]] == "hor"} {
            if {$upper_enc_height < $max_upper_enclosure} {
                set xTopEnc $max_upper_enclosure
                if {$upper_enc_width > $xTopEnc} {
                    set xTopEnc $upper_enc_width
                }
            } else {
                set xTopEnc $upper_enc_width
            }
            set yTopEnc $upper_enc_height
        } else {
            set xTopEnc $upper_enc_width
            if {$upper_enc_width < $max_upper_enclosure} {
                set yTopEnc $max_upper_enclosure
                if {$upper_enc_height > $yTopEnc} {
                    set yTopEnc $upper_enc_height
                }
            } else {
                set yTopEnc $upper_enc_height
            }
        }
        
        set rule [list \
		      rule $rule_name \
		      cutsize [dict get $via_info cut size] \
		      layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] [dict get $via_info upper layer]] \
		      cutspacing [list \
				      [expr [lindex [dict get $via_info cut spacing] 0] - [lindex [dict get $via_info cut size] 0]] \
				      [expr [lindex [dict get $via_info cut spacing] 1] - [lindex [dict get $via_info cut size] 1]] \
				     ] \
		      rowcol [list $rows $columns] \
		      enclosure [list $xBotEnc $yBotEnc $xTopEnc $yTopEnc] \
		     ]
        
        return $rule
    }
    
    proc get_viarule_name {lower x y width height} {
        set rules [select_viainfo $lower]
        set first_key [lindex [dict keys $rules] 0]
        set cut_layer [dict get $rules $first_key cut layer]

        return ${cut_layer}_${width}x${height}
    }
    
    proc get_cut_area {rule} {
        return [expr [lindex [dict get $rule rowcol] 0] * [lindex [dict get $rule rowcol] 0] * [lindex [dict get $rule cutsize] 0] * [lindex [dict get $rule cutsize] 1]]
    }
    
    proc select_rule {rule1 rule2} {
        if {[get_cut_area $rule2] > [get_cut_area $rule1]} {
            return $rule2
        }
        return $rule1
    }
    
    proc get_via {lower x y width height} {
        # First cur will assume that all crossing points (x y) are on grid for both lower and upper layers
        # TODO: Refine the algorithm to cope with offgrid intersection points
        variable physical_viarules
        
        set rule_name [get_viarule_name $lower $x $y $width $height]

        if {![dict exists $physical_viarules $rule_name]} {
            set selected_rule {}

            dict for {name rule} [select_viainfo $lower] {
                set result [get_via_option [get_dir $lower] $name $rule $x $y $width $height]
                if {$selected_rule == {}} {
                    set selected_rule $result
                } else {
                    # Choose the best between selected rule and current result, the winner becomes the new selected rule
                    set selected_rule [select_rule $selected_rule $result]
                }
            }

            dict set physical_viarules $rule_name $selected_rule
        }        
        
        return $rule_name
    }
    
    proc generate_vias {layer1 layer2 intersections} {
        variable logical_viarules
        variable physical_viarules
        variable metal_layers

        set vias {}
        set layer1_name $layer1
        set layer2_name $layer2
        regexp {(.*)_PIN_(hor|ver)} $layer1 - layer1_name layer1_direction
        
        set i1 [lsearch $metal_layers $layer1_name]
        set i2 [lsearch $metal_layers $layer2_name]
        if {$i1 == -1} {puts "Layer1 [dict get $connect layer1], Layer2 $layer2"; exit -1}
        if {$i2 == -1} {puts "Layer1 [dict get $connect layer1], Layer2 $layer2"; exit -1}

	# For each layer between l1 and l2, add vias at the intersection
        foreach intersection $intersections {
            if {![dict exists $logical_viarules [dict get $intersection rule]]} {
                puts "Missing key [dict get $intersection rule]"
                puts "Available keys [dict keys $logical_viarules]"
                exit -1
            }
            set logical_rule [dict get $logical_viarules [dict get $intersection rule]]

            set x [dict get $intersection x]
            set y [dict get $intersection y]
            set width  [dict get $logical_rule width]
            set height  [dict get $logical_rule height]
            
            set connection_layers [list $layer1 {*}[lrange $metal_layers [expr $i1 + 1] [expr $i2 - 1]]]
	    foreach lay $connection_layers {
                set via_name [get_via $lay $x $y $width $height]

                lappend vias [list name $via_name lower_layer $lay x [expr round([dict get $intersection x])] y [expr round([dict get $intersection y])]]
	    }
	}
	
        return $vias
    }

    ## Proc to generate via locations, both for a normal via and stacked via
    proc generate_via_stacks {l1 l2 tag grid_data} {
	variable logical_viarules
	variable default_grid_data
	variable orig_stripe_locs
	variable def_units
	
	set blockage [dict get $grid_data blockage]
	set area [dict get $grid_data area]
	
	#this variable contains locations of intersecting points of two orthogonal metal layers, between which via needs to be inserted
	#for every intersection. Here l1 and l2 are layer names, and i1 and i2 and their indices, tag represents domain (power or ground)	
	set intersections ""
	#check if layer pair is orthogonal, case 1
	set layer1 $l1
	if {[dict exists $grid_data layers $layer1]} {
	    set layer1_direction [get_dir $layer1]
	    set layer1_width [dict get $grid_data layers $layer1 width]
	    set layer1_width [expr round($layer1_width * $def_units)]
	} elseif {[regexp {(.*)_PIN_(hor|ver)} $l1 - layer1 layer1_direction]} {
	    #
	} else {
	    puts "Invalid direction for layer $l1"
	}
	
	set layer2 $l2
	if {[dict exists $grid_data layers $layer2]} {
	    set layer2_width [dict get $grid_data layers $layer2 width]
	    set layer2_width [expr round($layer2_width * $def_units)]
	} elseif {[dict exists $default_grid_data layers $layer2]} {
	    set layer2_width [dict get $default_grid_data layers $layer2 width]
	    set layer2_width [expr round($layer2_width * $def_units)]
	} else {
	    puts "No width information available for layer $layer2"
	}
	
	set ignore_count 0
	
	if {$layer1_direction == "hor" && [get_dir $l2] == "ver"} {

	    if {[array names orig_stripe_locs "$l1,$tag"] != ""} {
		## puts "Checking [llength $orig_stripe_locs($l1,$tag)] horizontal stripes on $l1, $tag"
		## puts "  versus [llength $orig_stripe_locs($l2,$tag)] vertical   stripes on $l2, $tag"
		## puts "     and [llength $blockage] blockages"
		#loop over each stripe of layer 1 and layer 2 
		foreach l1_str $orig_stripe_locs($l1,$tag) {
		    set a1  [expr {[lindex $l1_str 1]}]

		    foreach l2_str $orig_stripe_locs($l2,$tag) {
			set flag 1
			set a2	[expr {[lindex $l2_str 0]}]

			# Ignore if outside the area
			if {!($a2 >= [lindex $area 0] && $a2 <= [lindex $area 2] && $a1 >= [lindex $area 1] && $a1 <= [lindex $area 3])} {continue}
			if {$a2 > [lindex $l1_str 2] || $a2 < [lindex $l1_str 0]} {continue}
			if {$a1 > [lindex $l2_str 2] || $a1 < [lindex $l2_str 1]} {continue}

			if {[lindex $l2_str 1] == [lindex $area 3]} {continue}
			if {[lindex $l2_str 2] == [lindex $area 1]} {continue}

			#loop over each blockage geometry (macros are blockages)
			foreach blk1 $blockage {
			    set b1 [get_instance_llx $blk1]
			    set b2 [get_instance_lly $blk1]
			    set b3 [get_instance_urx $blk1]
			    set b4 [get_instance_ury $blk1]
			    ## Check if stripes are to be blocked on these blockages (blockages are specific to each layer). If yes, do not drop vias
			    if {  [lsearch [get_macro_blockage_layers $blk1] $l1] >= 0 || [lsearch [get_macro_blockage_layers $blk1] $l2] >= 0 } {
				if {($a2 > $b1 && $a2 < $b3 && $a1 > $b2 && $a1 < $b4 ) } {
				    set flag 0
				    break
				} 
				if {$a2 > $b1 && $a2 < $b3 && $a1 == $b2 && $a1 == [lindex $area 1]} {
				    set flag 0
				    break
				} 
				if {$a2 > $b1 && $a2 < $b3 && $a1 == $b4 && $a1 == [lindex $area 3]} {
				    set flag 0
				    break
				} 
			    }
			}

			if {$flag == 1} {
			    ## if no blockage restriction, append intersecting points to this "intersections"
			    if {[regexp {.*_PIN_(hor|ver)} $l1 - dir]} {
				set layer1_width [lindex $l1_str 3] ; # Already in def units
			    }
			    set rule_name ${l1}${layer2}_${layer2_width}x${layer1_width}
			    if {![dict exists $logical_viarules $rule_name]} {
				dict set logical_viarules $rule_name [list lower $l1 upper $layer2 width ${layer2_width} height ${layer1_width}]
			    }
			    lappend intersections "rule $rule_name x $a2 y $a1"
			}
		    }
		}
	    }

	} elseif {$layer1_direction == "ver" && [get_dir $l2] == "hor"} {
	    ##Second case of orthogonal intersection, similar criteria as above, but just flip of coordinates to find intersections
	    if {[array names orig_stripe_locs "$l1,$tag"] != ""} {
		## puts "Checking [llength $orig_stripe_locs($l1,$tag)] vertical   stripes on $l1, $tag"
		## puts "  versus [llength $orig_stripe_locs($l2,$tag)] horizontal stripes on $l2, $tag"
		## puts "     and [llength $blockage] blockages"
		foreach l1_str $orig_stripe_locs($l1,$tag) {
		    set n1  [expr {[lindex $l1_str 0]}]

		    foreach l2_str $orig_stripe_locs($l2,$tag) {
			set flag 1
			set n2	[expr {[lindex $l2_str 1]}]

			# Ignore if outside the area
			if {!($n1 >= [lindex $area 0] && $n1 <= [lindex $area 2] && $n2 >= [lindex $area 1] && $n2 <= [lindex $area 3])} {continue}
			if {$n2 > [lindex $l1_str 2] || $n2 < [lindex $l1_str 1]} {continue}
			if {$n1 > [lindex $l2_str 2] || $n1 < [lindex $l2_str 0]} {continue}

			foreach blk1 $blockage {
			    set b1 [get_instance_llx $blk1]
			    set b2 [get_instance_lly $blk1]
			    set b3 [get_instance_urx $blk1]
			    set b4 [get_instance_ury $blk1]
			    if {  [lsearch [get_macro_blockage_layers $blk1] $l1] >= 0 || [lsearch [get_macro_blockage_layers $blk1] $l2] >= 0 } {
				if {($n1 >= $b1 && $n1 <= $b3 && $n2 >= $b2 && $n2 <= $b4)} {
				    set flag 0	
				}
			    }
			}

			if {$flag == 1} {
                            ## if no blockage restriction, append intersecting points to this "intersections"
                            if {[regexp {.*_PIN_(hor|ver)} $l1 - dir]} {
                                set layer1_width [lindex $l1_str 3] ; # Already in def units
                            }
                            set rule_name ${l1}${layer2}_${layer1_width}x${layer2_width}
                            if {![dict exists $logical_viarules $rule_name]} {
                                dict set logical_viarules $rule_name [list lower $l1 upper $layer2 width ${layer1_width} height ${layer2_width}]
                            }
			    lappend intersections "rule $rule_name x $n1 y $n2"
			}


		    }
		}
	    }
	} else { 
	    #Check if stripes have orthogonal intersections. If not, exit
	    puts "ERROR: Adding vias between same direction layers is not supported yet."
	    puts "Layer: $l1, Direction: $layer1_direction"
	    puts "Layer: $l2, Direction: [get_dir $l2]"
	    exit
	}

	return [generate_vias $l1 $l2 $intersections]
    }

    # proc to generate follow pin layers or standard cell rails

    proc generate_lower_metal_followpin_rails {tag area} {
	variable orig_stripe_locs
	variable stripe_locs
	variable row_height
	variable rails_start_with

	#Assumes horizontal stripes
	set lay [get_rails_layer]

	if {$tag == $rails_start_with} { ;#If starting from bottom with this net, 
	    set lly [lindex $area 1]
	} else {
	    set lly [expr {[lindex $area 1] + $row_height}]
	}
	lappend stripe_locs($lay,$tag) "[lindex $area 0] $lly [lindex $area 2]"
	lappend orig_stripe_locs($lay,$tag) "[lindex $area 0] $lly [lindex $area 2]"


	#Rail every alternate rows - Assuming horizontal rows and full width rails
	for {set y [expr {$lly + (2 * $row_height)}]} {$y <= [lindex $area 3]} {set y [expr {$y + (2 * $row_height)}]} {
	    lappend stripe_locs($lay,$tag) "[lindex $area 0] $y [lindex $area 2]"
	    lappend orig_stripe_locs($lay,$tag) "[lindex $area 0] $y [lindex $area 2]"
	}
    }


    # proc for creating pdn mesh for upper metal layers
    proc generate_upper_metal_mesh_stripes {tag layer area} {
	variable widths
	variable pitches
	variable loffset
	variable boffset
	variable orig_stripe_locs
	variable stripe_locs
	variable stripes_start_with
	
	if {[get_dir $layer] == "hor"} {
	    set offset [expr [lindex $area 1] + $boffset($layer)]
	    if {$tag != $stripes_start_with} { ;#If not starting from bottom with this net, 
		set offset [expr {$offset + ($pitches($layer) / 2)}]
	    }
	    for {set y $offset} {$y < [expr {[lindex $area 3] - $widths($layer)}]} {set y [expr {$pitches($layer) + $y}]} {
		lappend stripe_locs($layer,$tag) "[lindex $area 0] $y [lindex $area 2]"
		lappend orig_stripe_locs($layer,$tag) "[lindex $area 0] $y [lindex $area 2]"
	    }
	} elseif {[get_dir $layer] == "ver"} {
	    set offset [expr [lindex $area 0] + $loffset($layer)]

	    if {$tag != $stripes_start_with} { ;#If not starting from bottom with this net, 
		set offset [expr {$offset + ($pitches($layer) / 2)}]
	    }
	    for {set x $offset} {$x < [expr {[lindex $area 2] - $widths($layer)}]} {set x [expr {$pitches($layer) + $x}]} {
		lappend stripe_locs($layer,$tag) "$x [lindex $area 1] [lindex $area 3]"
		lappend orig_stripe_locs($layer,$tag) "$x [lindex $area 1] [lindex $area 3]"
	    }
	} else {
	    puts "ERROR: Invalid direction \"[get_dir $layer]\" for metal layer ${layer}. Should be either \"hor\" or \"ver\". EXITING....."
	    exit
	}
    }

    # this proc chops down metal stripes wherever they are to be blocked
    # inputs to this proc are layer name, domain (tag), and blockage bbox cooridnates

    proc generate_metal_with_blockage {layer area tag b1 b2 b3 b4} {
	variable stripe_locs
	set temp_locs($layer,$tag) ""
	set temp_locs($layer,$tag) $stripe_locs($layer,$tag)
	set stripe_locs($layer,$tag) ""
	foreach l_str $temp_locs($layer,$tag) {
	    set loc1 [lindex $l_str 0]
	    set loc2 [lindex $l_str 1]
	    set loc3 [lindex $l_str 2]
	    location_stripe_blockage $loc1 $loc2 $loc3 $layer $area $tag $b1 $b2 $b3 $b4
	}
	
        set stripe_locs($layer,$tag) [lsort -unique $stripe_locs($layer,$tag)]
    }

    # sub proc called from previous proc
    proc location_stripe_blockage {loc1 loc2 loc3 lay area tag b1 b2 b3 b4} {
	variable widths
	variable stripe_locs

        set area_llx [lindex $area 0]
        set area_lly [lindex $area 1]
        set area_urx [lindex $area 2]
        set area_ury [lindex $area 3]

	if {[get_dir $lay] == "hor"} {
	    ##Check if stripe is passing through blockage
	    ##puts "HORIZONTAL BLOCKAGE "
	    set x1 $loc1
	    set y1 [expr $loc2 - $widths($lay)/2]
	    if {[lindex $area 1] > $y1} {
		set y1 [lindex $area 1]
	    }
	    set y1 [expr $loc2 - $widths($lay)/2]
	    set x2 $loc3
	    set y2 [expr $y1 +  $widths($lay)]
	    if {[lindex $area 3] < $y2} {
		set y2 [lindex $area 3]
	    }

	    #puts "segment:  [format {%9.1f %9.1f} $loc1 $loc3]"              
	    #puts "blockage: [format {%9.1f %9.1f} $b1 $b3]"
	    if {  ($y1 >= $b2) && ($y2 <= $b4) && ( ($x1 <= $b3 && $x2 >= $b3) || ($x1 <= $b1 && $x2 >= $b1)  || ($x1 <= $b1 && $x2 >= $b3) || ($x1 <= $b3 && $x2 >= $b1) )  } {

		if {$x1 <= $b1 && $x2 >= $b3} {	
		    #puts "  CASE3 of blockage in between left and right edge of core, cut the stripe into two segments"
		    #puts "    $x1 $loc2 $b1"
		    #puts "    $b3 $loc2 $x2"
		    lappend stripe_locs($lay,$tag) "$x1 $loc2 $b1"
		    lappend stripe_locs($lay,$tag) "$b3 $loc2 $x2"	
		} elseif {$x1 <= $b3 && $x2 >= $b3} {	
		    #puts "  CASE3 of blockage in between left and right edge of core, but stripe extending out only in one side (right)"
		    #puts "    $b3 $loc2 $x2"
		    lappend stripe_locs($lay,$tag) "$b3 $loc2 $x2"	
		} elseif {$x1 <= $b1 && $x2 >= $b1} {	
		    #puts "  CASE3 of blockage in between left and right edge of core, but stripe extending out only in one side (left)"
		    #puts "    $x1 $loc2 $b1"
		    lappend stripe_locs($lay,$tag) "$x1 $loc2 $b1"
		} else {
		    #puts "  CASE5 no match - eliminated segment"
		    #puts "    $loc1 $loc2 $loc3"
		}
	    } else {
		lappend stripe_locs($lay,$tag) "$x1 $loc2 $x2"
		#puts "stripe does not pass thru any layer blockage --- CASE 4 (do not change the stripe location)"
	    }
	}

	if {[get_dir $lay] == "ver"} {
	    ##Check if veritcal stripe is passing through blockage, same strategy as above
	    set x1 $loc1 ;# [expr max($loc1 -  $widths($lay)/2, [lindex $area 0])]
	    set y1 $loc2
	    set x2 $loc1 ;# [expr min($loc1 +  $widths($lay)/2, [lindex $area 2])]
	    set y2 $loc3

	    if {$x2 > $b1 && $x1 < $b3} {

		if {$y1 <= $b2 && $y2 >= $b4} {	
		    ##puts "CASE3 of blockage in between top and bottom edge of core, cut the stripe into two segments
		    lappend stripe_locs($lay,$tag) "$loc1 $y1 $b2"
		    lappend stripe_locs($lay,$tag) "$loc1 $b4 $y2"	
		} elseif {$y1 <= $b4 && $y2 >= $b4} {	
		    ##puts "CASE3 of blockage in between top and bottom edge of core, but stripe extending out only in one side (right)"
		    lappend stripe_locs($lay,$tag) "$loc1 $b4 $y2"	
		} elseif {$y1 <= $b2 && $y2 >= $b2} {	
		    ##puts "CASE3 of blockage in between top and bottom edge of core, but stripe extending out only in one side (left)"
		    lappend stripe_locs($lay,$tag) "$loc1 $y1 $b2"
		} elseif {$y1 <= $b4 && $y1 >= $b2 && $y2 >= $b2 && $y2 <= $b4} {	
		    ##completely enclosed - remove segment
		} else {
		    #puts "  CASE5 no match"
		    #puts "    $loc1 $loc2 $loc3"
		    lappend stripe_locs($lay,$tag) "$loc1 $y1 $y2"
		}
	    } else {
		lappend stripe_locs($lay,$tag) "$loc1 $y1 $y2"
	    }
	}
    }


    ## this is a top-level proc to generate PDN stripes and insert vias between these stripes
    proc generate_stripes_vias {tag net_name grid_data} {
        variable vias
        
        set area [dict get $grid_data area]
        set blockage [dict get $grid_data blockage]

	## puts "Adding stripes for $net_name ..."
	foreach lay [dict keys [dict get $grid_data layers]] {
	    ## puts "    Layer $lay ..."

	    if {$lay == [get_rails_layer]} {
	        #Std. cell rails
	        generate_lower_metal_followpin_rails $tag $area

	        foreach blk1 $blockage {
		    set b1 [get_instance_llx $blk1]
		    set b2 [get_instance_lly $blk1]
		    set b3 [get_instance_urx $blk1]
		    set b4 [get_instance_ury $blk1]
		    generate_metal_with_blockage [get_rails_layer] $area $tag $b1 $b2 $b3 $b4
	        }

            } else {
	        #Upper layer stripes
		generate_upper_metal_mesh_stripes $tag $lay $area

		foreach blk2 $blockage {
		    if {  [lsearch [get_macro_blockage_layers $blk2] $lay] >= 0 } {
			set c1 [get_instance_llx $blk2]
			set c2 [get_instance_lly $blk2]
			set c3 [get_instance_urx $blk2]
			set c4 [get_instance_ury $blk2]

			generate_metal_with_blockage $lay $area $tag $c1 $c2 $c3 $c4
		    }
		}
	    }
	}

	#Via stacks
	## puts "Adding vias for $net_name ([llength [dict get $grid_data connect]] connections)..."
	foreach tuple [dict get $grid_data connect] {
	    set l1 [lindex $tuple 0]
	    set l2 [lindex $tuple 1]
	    ## puts "    $l1 to $l2"
	    set connections [generate_via_stacks $l1 $l2 $tag $grid_data]
	    lappend vias [list net_name $net_name connections $connections]
	}
    }

    namespace export write_def write_vias

    variable macros {}
    variable instances {}
    
    proc get_macro_boundaries {} {
        variable instances

        set boundaries {}
        foreach instance [dict keys $instances] {
            lappend boundaries [dict get $instances $instance macro_boundary]
        }
        
        return $boundaries
    }

    proc get_macro_halo_boundaries {} {
        variable instances

        set boundaries {}
        foreach instance [dict keys $instances] {
            lappend boundaries [dict get $instances $instance halo_boundary]
        }
        
        return $boundaries
    }
    
    proc read_macro_boundaries {} {
        variable libs
        variable macros
        variable instances

        foreach lib $libs {
            foreach cell [$lib getMasters] {
                if {[$cell getType] == "CORE"} {continue}
                if {[$cell getType] == "IO"} {continue}
                if {[$cell getType] == "PAD"} {continue}
                if {[$cell getType] == "PAD_SPACER"} {continue}
                if {[$cell getType] == "SPACER"} {continue}
                if {[$cell getType] == "NONE"} {continue}
                if {[$cell getType] == "ENDCAP_PRE"} {continue}
                if {[$cell getType] == "ENDCAP_BOTTOMLEFT"} {continue}
                if {[$cell getType] == "ENDCAP_BOTTOMRIGHT"} {continue}
                if {[$cell getType] == "ENDCAP_TOPLEFT"} {continue}
                if {[$cell getType] == "ENDCAP_TOPRIGHT"} {continue}
                if {[$cell getType] == "ENDCAP"} {continue}
                if {[$cell getType] == "CORE_SPACER"} {continue}
                if {[$cell getType] == "CORE_TIEHIGH"} {continue}
                if {[$cell getType] == "CORE_TIELOW"} {continue}

                dict set macros [$cell getName] [list \
						     width  [$cell getWidth] \
						     height [$cell getHeight] \
						    ]
            }
        }

        set instances [read_def_components [dict keys $macros]]

        foreach instance [dict keys $instances] {
            set macro_name [dict get $instances $instance macro]

            set llx [dict get $instances $instance xmin]
            set lly [dict get $instances $instance ymin]
            set urx [dict get $instances $instance xmax]
            set ury [dict get $instances $instance ymax]

            dict set instances $instance macro_boundary [list $llx $lly $urx $ury]

            set halo [dict get $instances $instance halo]
            set llx [expr round($llx - [lindex $halo 0])]
            set lly [expr round($lly - [lindex $halo 1])]
            set urx [expr round($urx + [lindex $halo 2])]
            set ury [expr round($ury + [lindex $halo 3])]

            dict set instances $instance halo_boundary [list $llx $lly $urx $ury]
        }
    }

    proc read_def_components {macros} {
        variable design_data
        variable block
        set instances {}

        foreach inst [$block getInsts] {
            set macro_name [[$inst getMaster] getName]
            if {[lsearch $macros $macro_name] != -1} {
                set data {}
                dict set data name [$inst getName]
                dict set data macro $macro_name
                dict set data x [lindex [$inst getOrigin] 0]
                dict set data y [lindex [$inst getOrigin] 1]
                dict set data xmin [[$inst getBBox] xMin]
                dict set data ymin [[$inst getBBox] yMin]
                dict set data xmax [[$inst getBBox] xMax]
                dict set data ymax [[$inst getBBox] yMax]
                dict set data orient [$inst getOrient]

                if {[$inst getHalo] != "NULL"} {
                    dict set data halo [list \
					    [[$inst getHalo] xMin] \
					    [[$inst getHalo] yMin] \
					    [[$inst getHalo] xMax] \
					    [[$inst getHalo] yMax] \
					   ]
                } else {
                    dict set data halo [dict get $design_data config default_halo]
                }

                dict set instances [$inst getName] $data
            }
        }

        return $instances
    }


    variable global_connections {
        VDD {
            {inst_name .* pin_name VDD}
            {inst_name .* pin_name VDDPE}
            {inst_name .* pin_name VDDCE}
        }
        VSS {
            {inst_name .* pin_name VSS}
            {inst_name .* pin_name VSSE}
        }
    }
    
    proc write_opendb_vias {} {
        variable physical_viarules
        variable block
        variable tech

        dict for {name rule} $physical_viarules {
            set via [odb::dbVia_create $block $name]
            $via setViaGenerateRule [$tech findViaGenerateRule [dict get $rule rule]]
            set params [$via getViaParams]
            $params setBottomLayer [$tech findLayer [lindex [dict get $rule layers] 0]]
            $params setCutLayer [$tech findLayer [lindex [dict get $rule layers] 1]]
            $params setTopLayer [$tech findLayer [lindex [dict get $rule layers] 2]]
            $params setXCutSize [lindex [dict get $rule cutsize] 0]
            $params setYCutSize [lindex [dict get $rule cutsize] 1]
            $params setXCutSpacing [lindex [dict get $rule cutspacing] 0]
            $params setYCutSpacing [lindex [dict get $rule cutspacing] 1]
            $params setXBottomEnclosure [lindex [dict get $rule enclosure] 0]
            $params setYBottomEnclosure [lindex [dict get $rule enclosure] 1]
            $params setXTopEnclosure [lindex [dict get $rule enclosure] 2]
            $params setYTopEnclosure [lindex [dict get $rule enclosure] 3]
            $params setNumCutRows [lindex [dict get $rule rowcol] 0]
            $params setNumCutCols [lindex [dict get $rule rowcol] 1]

            $via setViaParams $params
        }
    }

    proc write_opendb_specialnet {net_name signal_type} {
        variable block
        variable instances
        variable metal_layers
        variable tech 
        variable stripe_locs
        variable widths
        variable global_connections
        
        set net [$block findNet $net_name]
        if {$net == "NULL"} {
            set net [odb::dbNet_create $block $net_name]
        }
        $net setSpecial
        $net setSigType $signal_type

        foreach inst [$block getInsts] {
            set master [$inst getMaster]
            foreach mterm [$master getMTerms] {
                if {[$mterm getSigType] == $signal_type} {
                    foreach pattern [dict get $global_connections $net_name] {
                        if {[regexp [dict get $pattern inst_name] [$inst getName]] &&
                            [regexp [dict get $pattern pin_name] [$mterm getName]]} {
                            odb::dbITerm_connect $inst $net $mterm
                        }
                    }
                }
            }
            foreach iterm [$inst getITerms] {
                if {[$iterm getNet] != "NULL" && [[$iterm getNet] getName] == $net_name} {
                    $iterm setSpecial
                }
            }
        }
        $net setWildConnected
        set swire [odb::dbSWire_create $net "ROUTED"]

        foreach lay $metal_layers {
            set layer [$tech findLayer $lay]

            set dir [get_dir $lay]
            if {$dir == "hor"} {
                foreach l_str $stripe_locs($lay,$signal_type) {
                    set l1 [lindex $l_str 0]
                    set l2 [lindex $l_str 1]
                    set l3 [lindex $l_str 2]
                    if {$l1 == $l3} {continue}
                    if {$lay == [get_rails_layer]} {
                        odb::dbSBox_create $swire $layer [expr round($l1)] [expr round($l2 - ($widths($lay)/2))] [expr round($l3)] [expr round($l2 + ($widths($lay)/2))] "FOLLOWPIN"
                    } else {
                        odb::dbSBox_create $swire $layer [expr round($l1)] [expr round($l2 - ($widths($lay)/2))] [expr round($l3)] [expr round($l2 + ($widths($lay)/2))] "STRIPE"
                    }

                }
            } elseif {$dir == "ver"} {
                foreach l_str $stripe_locs($lay,$signal_type) {
                    set l1 [lindex $l_str 0]
                    set l2 [lindex $l_str 1]
                    set l3 [lindex $l_str 2]
                    if {$l2 == $l3} {continue}
                    if {$lay == [get_rails_layer]} {
                        odb::dbSBox_create $swire $layer [expr round($l1 - ($widths($lay)/2))] [expr round($l2)] [expr round($l1 + ($widths($lay)/2))] [expr round($l3)] "FOLLOWPIN"
                    } else {
                        odb::dbSBox_create $swire $layer [expr round($l1 - ($widths($lay)/2))] [expr round($l2)] [expr round($l1 + ($widths($lay)/2))] [expr round($l3)] "STRIPE"
                    }
                }               
            }
        }

        variable vias
        foreach via $vias {
            if {[dict get $via net_name] == $net_name} {
	        # For each layer between l1 and l2, add vias at the intersection
                foreach via_inst [dict get $via connections] {
                    set via_name [dict get $via_inst name]
                    set x        [dict get $via_inst x]
                    set y        [dict get $via_inst y]
                    set lay      [dict get $via_inst lower_layer]
                    regexp {(.*)_PIN} $lay - lay
                    set layer [$tech findLayer $lay]
                    odb::dbSBox_create $swire [$block findVia $via_name] $x $y "STRIPE"
	        }
            }
        }
    }
    
    proc write_opendb_specialnets {} {
        variable block
        variable design_data
        
        foreach net_name [dict get $design_data power_nets] {
            write_opendb_specialnet "VDD" "POWER"
        }

        foreach net_name [dict get $design_data ground_nets] {
            write_opendb_specialnet "VSS" "GROUND"
        }
        
    }
    
    proc init_orientation {height} {
        variable lowest_rail
        variable orient_rows
        variable rails_start_with

        set lowest_rail $height
        if {$rails_start_with == "GROUND"} {
            set orient_rows {0 "R0" 1 "MX"}
        } else {
            set orient_rows {0 "MX" 1 "R0"}
        }
    }

    proc orientation {height} {
        variable lowest_rail
        variable orient_rows
        variable row_height
        
        set row_line [expr int(($height - $lowest_rail) / $row_height)]
        return [dict get $orient_rows [expr $row_line % 2]]
    }
    
    proc write_opendb_row {height start end} {
        variable row_index
        variable block
        variable site
        variable site_width
        variable def_units
        variable design_data
        
        set start  [expr int($start)]
        set height [expr int($height)]
        set end    [expr int($end)]

        if {$start == $end} {return}
        
        set llx [lindex [dict get $design_data config core_area] 0]
	if {[expr { int($start - $llx) % $site_width}] == 0} {
	    set x $start
	} else {
	    set offset [expr { int($start - $llx) % $site_width}]
	    set x [expr {$start + $site_width - $offset}]
	}

	set num  [expr {($end - $x)/$site_width}]
        
        odb::dbRow_create $block ROW_$row_index $site $x $height [orientation $height] "HORIZONTAL" $num $site_width
        incr row_index
    }

    proc write_opendb_rows {} {
        variable stripe_locs
        variable row_height
        variable row_index

        set row_index 1
        
        set stripes [concat $stripe_locs([get_rails_layer],POWER) $stripe_locs([get_rails_layer],GROUND)]
        set new_stripes {}
        foreach stripe $stripes {
            if {[lindex $stripe 0] != [lindex $stripe 2]} {
                lappend new_stripes $stripe
            }
        }
        set stripes $new_stripes
        set stripes [lsort -real -index 1 $stripes]
        set heights {}
        foreach stripe $stripes {
            lappend heights [lindex $stripe 1]
        }
        set heights [lsort -unique -real $heights]

        init_orientation [lindex $heights 0]

	foreach height [lrange $heights 0 end-1] {
            set rails {}
            foreach stripe $stripes {
                if {[lindex $stripe 1] == $height} {
                    lappend rails $stripe
                }
            }
            set lower_rails [lsort -real -index 2 $rails]
            set rails {}
            foreach stripe $stripes {
                if {[lindex $stripe 1] == ($height + $row_height)} {
                    lappend rails $stripe
                }
            }
            set upper_rails [lsort -real -index 2 $rails]
            set upper_extents {}
            foreach upper_rail $upper_rails {
                lappend upper_extents [lindex $upper_rail 0]
                lappend upper_extents [lindex $upper_rail 2]
            }

            foreach lrail $lower_rails {
                set idx 0
                set start [lindex $lrail 0]
                set end   [lindex $lrail 2]

                # Find index of first number that is greater than the start position of this rail
                while {$idx < [llength $upper_extents]} {
                    if {[lindex $upper_extents $idx] > $start} {
                        break
                    }
                    incr idx
                }

                if {[lindex $upper_extents $idx] <= $start} {
                    continue
                }

                if {$idx % 2 == 0} {
                    # If the index is even, then the start of the rail has no matching rail above it
                    set row_start [lindex $upper_extents $idx]
                    incr idx
                } else {
                    # If the index is odd, then the start of the rail has matchin rail above it
                    set row_start $start
                }

                if {$end <= [lindex $upper_extents $idx]} {
                    write_opendb_row $height $row_start $end
                    
                } else {
                    while {$idx < [llength $upper_extents] && $end > [lindex $upper_extents [expr $idx + 1]]} {
                        write_opendb_row $height $row_start [lindex $upper_extents $idx]
                        set row_start [lindex $upper_extents [expr $idx + 1]]
                        set idx [expr $idx + 2]
                    }
                }
            }
        }
    }

    #
    ############################### Instructions #################################
    # To generate the DEF, enter all details in inputs.tcl file
    # Run 'tclsh create_pg_grid_v1.tcl'
    # DEF with power grid will be dumped out with the given name
    # Contact umallapp@ucsd.edu for any questions
    ##############################################################################

    ############################### To be improved ###############################
    # 1. Currently, generate_lower_metal_followpin_rails() handles only 1 power 
    #    net and 1 ground net
    # 2. Need to add support for designs with macros and rectilinear floorplans.
    # 3. Currently supports only one width and pitch per layer, for all 
    #    specialnets in that layer
    # 4. (a) Accepts only 1 viarule per metal layer and only adds that via per layer
    #    (b) Vias added at every intersection between two layers. Need to allow for
    #    	 a 'pitch' setting
    # 5. If offsets/widths/pitches result in off-track, enable snapping to grid
    # 6. Handle mask information
    ##############################################################################
    ###########################Improved in this version ##########################
    # 1. Separate inputs file
    # 2. Process techfile to get BEOL information
    # 3. Can create PG stripes, given a particular area of the chip
    ##############################################################################
    ##############################################################################

    variable block
    variable tech
    variable libs
    variable design_data {}
    variable default_grid_data {}
    variable def_output
    variable widths
    variable pitches
    variable loffset
    variable boffset
    variable site
    variable site_width
    variable site_name
    variable row_height
    variable metal_layers {}
    variable metal_layers_dir {}
    
    ## procedure for file existence check, returns 0 if file does not exist or file exists, but empty
    proc -s {filename} {
	return [expr {([file exists $filename]) && ([file size $filename] > 0)}]
    }

    proc get {args} {
        variable design_data
        
        return [dict get $design_data {*}$args]
    }
    proc get_macro_power_pins {inst_name} {
        set specification [select_instance_specification $inst_name]
        if {[dict exists $specification power_pins]} {
            return [dict get $specification power_pins]
        }
        return "VDDPE VDDCE"
    }
    proc get_macro_ground_pins {inst_name} {
        set specification [select_instance_specification $inst_name]
        if {[dict exists $specification ground_pins]} {
            return [dict get $specification ground_pins]
        }
        return "VSSE"
    }
    
    proc transform_box {xmin ymin xmax ymax origin orientation} {
        switch -exact $orientation {
            R0    {set new_box [list $xmin $ymin $xmax $ymax]}
            R90   {set new_box [list [expr -1 * $ymax] $xmin [expr -1 * $ymin] $xmax]}
            R180  {set new_box [list [expr -1 * $xmax] [expr -1 * $ymax] [expr -1 * $xmin] [expr -1 * $ymin]]}
            R270  {set new_box [list $ymin [expr -1 * $xmax] $ymax [expr -1 * $xmin]]}
            MX    {set new_box [list $xmin [expr -1 * $ymax] $xmax [expr -1 * $ymin]]}
            MY    {set new_box [list [expr -1 * $xmax] $ymin [expr -1 * $xmin] $ymax]}
            MXR90 {set new_box [list $ymin $xmin $ymax $xmax]}
            MYR90 {set new_box [list [expr -1 * $ymax] [expr -1 * $xmax] [expr -1 * $ymin] [expr -1 * $xmin]]}
            default {error "Illegal orientation $orientation specified"}
        }
        return [list \
		    [expr [lindex $new_box 0] + [lindex $origin 0]] \
		    [expr [lindex $new_box 1] + [lindex $origin 1]] \
		    [expr [lindex $new_box 2] + [lindex $origin 0]] \
		    [expr [lindex $new_box 3] + [lindex $origin 1]] \
		   ]
    }
    
    proc get_memory_instance_pg_pins {} {
        variable block
        variable orig_stripe_locs

        foreach inst [$block getInsts] {
            set inst_name [$inst getName]
            set master [$inst getMaster]

            if {[$master getType] == "CORE"} {continue}
            if {[$master getType] == "IO"} {continue}
            if {[$master getType] == "PAD"} {continue}
            if {[$master getType] == "PAD_SPACER"} {continue}
            if {[$master getType] == "SPACER"} {continue}
            if {[$master getType] == "NONE"} {continue}
            if {[$master getType] == "ENDCAP_PRE"} {continue}
            if {[$master getType] == "ENDCAP_BOTTOMLEFT"} {continue}
            if {[$master getType] == "ENDCAP_BOTTOMRIGHT"} {continue}
            if {[$master getType] == "ENDCAP_TOPLEFT"} {continue}
            if {[$master getType] == "ENDCAP_TOPRIGHT"} {continue}
            if {[$master getType] == "ENDCAP"} {continue}
            if {[$master getType] == "ENDCAP"} {continue}
            if {[$master getType] == "ENDCAP"} {continue}
            if {[$master getType] == "ENDCAP"} {continue}
            if {[$master getType] == "CORE_SPACER"} {continue}
            if {[$master getType] == "CORE_TIEHIGH"} {continue}
            if {[$master getType] == "CORE_TIELOW"} {continue}

            foreach term_name [concat [get_macro_power_pins $inst_name] [get_macro_ground_pins $inst_name]] {
                set inst_term [$inst findITerm $term_name]
                if {$inst_term == "NULL"} {continue}
                
                set mterm [$inst_term getMTerm]
                set type [$mterm getSigType]

                foreach mPin [$mterm getMPins] {
                    foreach geom [$mPin getGeometry] {
                        set layer [[$geom getTechLayer] getName]
                        set box [transform_box [$geom xMin] [$geom yMin] [$geom xMax] [$geom yMax] [$inst getOrigin] [$inst getOrient]]

                        set width  [expr abs([lindex $box 2] - [lindex $box 0])]
                        set height [expr abs([lindex $box 3] - [lindex $box 1])]

                        if {$width > $height} {
                            set xl [lindex $box 0]
                            set xu [lindex $box 2]
                            set y  [expr ([lindex $box 1] + [lindex $box 3])/2]
                            set width [expr abs([lindex $box 3] - [lindex $box 1])]
                            lappend orig_stripe_locs(${layer}_PIN_hor,$type) [list $xl $y $xu $width]
                        } else {
                            set x  [expr ([lindex $box 0] + [lindex $box 2])/2]
                            set yl [lindex $box 1]
                            set yu [lindex $box 3]
                            set width [expr abs([lindex $box 2] - [lindex $box 0])]
                            lappend orig_stripe_locs(${layer}_PIN_ver,$type) [list $x $yl $yu $width]
                        }
                    }
                }
            }    
        }
	#        puts "Total walltime till macro pin geometry creation = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"
    }

    proc set_core_area {xmin ymin xmax ymax} {
        variable design_data

        dict set design_data config core_area [list $xmin $ymin $xmax $ymax]
    }

    proc init {opendb_block {PDN_cfg "PDN.cfg"}} {
        variable db
        variable block
        variable tech
        variable libs
        variable design_data
        variable def_output
        variable default_grid_data
        variable stripe_locs
        variable design_name
        variable site
        variable row_height
        variable site_width
        variable site_name
        variable metal_layers
        variable def_units
        variable stripes_start_with
        variable rails_start_with
        
	#        set ::start_time [clock clicks -milliseconds]
        if {![-s $PDN_cfg]} {
	    puts "File $PDN_cfg does not exist, or exists but empty"
	    exit 1
        }

        source $PDN_cfg

        set block $opendb_block
        set def_units [$block getDefUnits]
        set design_name [$block getName]
        set db [$block getDataBase]
        set tech [$db getTech]
        set libs [$db getLibs]

        init_metal_layers
        init_via_tech
        
        set die_area [$block getDieArea]
        puts "Design Name is $design_name"
        set def_output "${design_name}_pdn.def"
        
        if {[info vars ::power_nets] == ""} {
            set ::power_nets "VDD"
        }
        
        if {[info vars ::ground_nets] == ""} {
            set ::ground_nets "VSS"
        }

        if {[info vars ::stripes_start_with] == ""} {
            set stripes_start_with "GROUND"
        } else {
            set stripes_start_with $::stripes_start_with
        }
        
        if {[info vars ::rails_start_with] == ""} {
            set rails_start_with "GROUND"
        } else {
            set rails_start_with $::rails_start_with
        }
        
        dict set design_data power_nets $::power_nets
        dict set design_data ground_nets $::ground_nets

        # Sourcing user inputs file
        #
        set sites {}
        foreach lib $libs {
            set sites [concat $sites [$lib getSites]]
        }
        set site [lindex $sites 0]

        set site_name [$site getName]
        set site_width [$site getWidth] 
        
        set row_height [$site getHeight]

        ##### Get information from BEOL LEF
        puts "Reading BEOL LEF and gathering information ..."

	#        puts " DONE \[Total elapsed walltime = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds\]"

        if {[info vars ::layers] != ""} {
            foreach layer $::layers {
                if {[dict exists $::layers $layer widthtable]} {
                    dict set ::layers $layer widthtable [lmap x [dict get $::layers $layer widthtable] {expr $x * $def_units}]
                }
            }
            set_layer_info $::layers
        }

        if {[info vars ::halo] != ""} {
            if {[llength $::halo] == 1} {
                set default_halo "$::halo $::halo $::halo $::halo"
            } elseif {[llength $::halo] == 2} {
                set default_halo "$::halo $::halo"
            } elseif {[llength $::halo] == 4} {
                set default_halo $::halo
	    } else {
                error "ERROR: Illegal number of elements defined for ::halo \"$::halo\""
            }
        } else {
            set default_halo "0 0 0 0"
        }

        dict set design_data config def_output   $def_output
        dict set design_data config design       $design_name
        dict set design_data config die_area     [list [$die_area xMin]  [$die_area yMin] [$die_area xMax] [$die_area yMax]]
        dict set design_data config default_halo [lmap x $default_halo {expr $x * $def_units}]
	
        if {[info vars ::core_area_llx] != "" && [info vars ::core_area_lly] != "" && [info vars ::core_area_urx] != "" && [info vars ::core_area_ury] != ""} {
	    set_core_area [expr $::core_area_llx * $def_units] [expr $::core_area_lly * $def_units] [expr $::core_area_urx * $def_units] [expr $::core_area_ury * $def_units]
        }
        
        foreach lay $metal_layers { 
	    set stripe_locs($lay,POWER) ""
	    set stripe_locs($lay,GROUND) ""
        }

        ########################################
        # Remove existing rows
        #######################################
        foreach row [$block getRows] {
            odb::dbRow_destroy $row
        }

        ########################################
        # Remove existing power/ground nets
        #######################################
        foreach pg_net [concat [dict get $design_data power_nets] [dict get $design_data ground_nets]] {
            set net [$block findNet $pg_net]
            if {$net != "NULL"} {
                odb::dbNet_destroy $net
            }
        }

        ########################################
        # Creating blockages based on macro locations
        #######################################
        read_macro_boundaries

        get_memory_instance_pg_pins

        if {$default_grid_data == {}} {
            set default_grid_data [lindex [dict get $design_data grid stdcell] 0]
        }

        ##### Basic sanity checks to see if inputs are given correctly
        if {[lsearch $metal_layers [get_rails_layer]] < 0} {
	    error "ERROR: Layer specified for std. cell rails not in list of layers."
        }

	#        puts "Total walltime till PDN setup = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"

        return $design_data
    }
    
    proc specify_grid {type specification} {
        variable design_data

        set specifications {}
        if {[dict exists $design_data grid $type]} {
            set specifications [dict get $design_data grid $type]
        }
        lappend specifications $specification
        
        dict set design_data grid $type $specifications
    }
    
    proc add_grid {grid_data} {
        variable design_data
        variable def_units
        variable widths
        variable pitches
        variable loffset
        variable boffset
        
        ##### Creating maps for directions, widths and pitches
        set area [dict get $grid_data area]

        foreach lay [dict keys [dict get $grid_data layers]] { 
	    set widths($lay)    [expr round([dict get $grid_data layers $lay width] * $def_units)] 
	    set pitches($lay)   [expr round([dict get $grid_data layers $lay pitch] * $def_units)]
	    set loffset($lay)   [expr round([dict get $grid_data layers $lay offset] * $def_units)]
	    set boffset($lay)   [expr round([dict get $grid_data layers $lay offset] * $def_units)]
        }

        ## Power nets
        ## puts "Power straps"
        foreach pwr_net [dict get $design_data power_nets] {
	    set tag "POWER"
	    generate_stripes_vias $tag $pwr_net $grid_data
        }
        ## Ground nets
        ## puts "Ground straps"
        foreach gnd_net [dict get $design_data ground_nets] {
	    set tag "GROUND"
	    generate_stripes_vias $tag $gnd_net $grid_data
        }

    }

    proc select_instance_specification {instance} {
        variable design_data
        variable instances

        set macro_specifications [dict get $design_data grid macro]
        
        # If there is a specifcation that matches this instance name, use that
        foreach specification $macro_specifications {
            if {![dict exists $specification instance]} {continue}
            if {[dict get $specification instance] == $instance} {
                return $specification
            }
        }
        
        # If there is a specification that matches this macro name, use that
        if {[dict exists $instances $instance]} {
            set instance_macro [dict get $instances $instance macro]

            # If there are orientation based specifcations for this macro, use the appropriate one if available
            foreach spec $macro_specifications {
                if {!([dict exists $spec macro] && [dict get $spec orient] && [dict get $spec macro] == $instance_macro)} {continue}
                if {[lsearch [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
                    return $spec
                }
            }
	    
            # There should only be one macro specific spec that doesnt have an orientation qualifier
            foreach spec $macro_specifications {
                if {!([dict exists $spec macro] && [dict get $spec macro] == $instance_macro)} {continue}
                if {[lsearch [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
                    return $spec
                }
            }

            # If there are orientation based specifcations, use the appropriate one if available
            foreach spec $macro_specifications {
                if {!(![dict exists $spec macro] && ![dict exists $spec instance] && [dict exists $spec orient])} {continue}
                if {[lsearch [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
                    return $spec
                }
            }
        }

        # There should only be one macro specific spec that doesnt have an orientation qualifier
        foreach spec $macro_specifications {
            if {!(![dict exists $spec macro] && ![dict exists $spec instance])} {continue}
            return $spec
        }

        error "Error: no matching grid specification found for $instance"
    }
    
    proc get_instance_specification {instance} {
        variable instances

        set specification [select_instance_specification $instance]

        if {![dict exists $specification blockage]} {
            dict set specification blockage {}
        }
        dict set specification area [dict get $instances $instance macro_boundary]
        
        return $specification
    }
    
    proc init_metal_layers {} {
        variable tech
        variable metal_layers
        variable metal_layers_dir

        set metal_layers {}        
        set metal_layers_dir {}
        
        foreach layer [$tech getLayers] {
            if {[$layer getType] == "ROUTING"} {
                lappend metal_layers [$layer getName]
                lappend metal_layers_dir [$layer getDirection]
            }
        }
    }

    proc get_instance_llx {instance} {
        variable instances
        return [lindex [dict get $instances $instance halo_boundary] 0]
    }
    
    proc get_instance_lly {instance} {
        variable instances
        return [lindex [dict get $instances $instance halo_boundary] 1]
    }
    
    proc get_instance_urx {instance} {
        variable instances
        return [lindex [dict get $instances $instance halo_boundary] 2]
    }
    
    proc get_instance_ury {instance} {
        variable instances
        return [lindex [dict get $instances $instance halo_boundary] 3]
    }
    
    proc get_macro_blockage_layers {instance} {
        variable metal_layers
        
        set specification [select_instance_specification $instance]
        if {[dict exists $specification blockage]} {
            return [dict get $specification blockage]
        }
        return [lrange $metal_layers 0 3]
    }
    
    proc print_strategy {type specification} {
        puts "Type: $type"
        if {[dict exists $specification rails]} {
            puts "    Follow Pins Layer: [dict get $specification rails]"
        }
        if {[dict exists $specification instance]} {
            puts "    Instance: [dict get $specification orient]"
        }
        if {[dict exists $specification macro]} {
            puts "    Macro: [dict get $specification orient]"
        }
        if {[dict exists $specification orient]} {
            puts "    Macro orientation: [dict get $specification orient]"
        }
        dict for {layer_name layer} [dict get $specification layers] {
            puts [format "    Layer: %s, Width: %f Pitch: %f Offset: %f" $layer_name [dict get $layer width]  [dict get $layer pitch] [dict get $layer offset]]
        }
        puts "    Connect: [dict get $specification connect]"
    }
    
    proc plan_grid {} {
        variable design_data
        variable instances
        variable default_grid_data
        variable def_units

        ################################## Main Code #################################

        puts "****** INFO ******"
        foreach specification [dict get $design_data grid stdcell] {
            print_strategy stdcell $specification
        }
        foreach specification [dict get $design_data grid macro] {
            print_strategy macro $specification
        }
        puts "**** END INFO ****"

        foreach specification [dict get $design_data grid stdcell] {
            if {[dict exists $specification name]} {
                puts "Inserting stdcell grid - [dict get $specification name]"
            } else {
                puts "Inserting stdcell grid"
            }
            dict set specification blockage [dict keys $instances]
            if {![dict exists $specification area]} {
                dict set specification area [dict get $design_data config core_area]
            }
            add_grid $specification
        }
        
        if {[llength [dict keys $instances]] > 0} {
            puts "Inserting macro grid for [llength [dict keys $instances]] macros"
            foreach instance [dict keys $instances] {
                add_grid [get_instance_specification $instance]
            }
        }
    }
    
    proc opendb_update_grid {} {
        puts "Writing to database"
        write_opendb_vias
        write_opendb_specialnets
        write_opendb_rows
    }
    
    proc apply_pdn {config verbose} {
	if {$verbose} {
	    puts "##Power Delivery Network Generator: $config $verbose"
	}
	set db [::ord::get_db]
	set block [[$db getChip] getBlock]
        init $block $config

        set ::start_time [clock clicks -milliseconds]
	if {$verbose} {
	    puts "##Power Delivery Network Generator: Generating PDN DEF"
	}

        plan_grid
        opendb_update_grid

	if {$verbose} {
	    puts "Total walltime to generate PDN DEF = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"
	}
    }


}

proc run_pdngen { args } {
  sta::parse_key_args "run_pdngen" args \
    keys {} flags {-verbose}

  set verbose [info exists flags(-verbose)]

  sta::check_argc_eq1 "run_pdngen" $args
  pdngen::apply_pdn $args $verbose
}
