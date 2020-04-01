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

#sta::define_cmd_args "pdngen" {[-verbose] config_file}

# Messages:
# 
# Information
# 8 "Design Name is $design_name"
# 9 "Reading technology data"
# 10 "Inserting macro grid for [llength [dict keys $instances]] macros"
# 11 "****** INFO ******"
# 12 "**** END INFO ****"
# 13 "Inserting stdcell grid - [dict get $specification name]"
# 14 "Inserting stdcell grid"
# 15 "Writing to database"
# 16 "##Power Delivery Network Generator: Generating PDN"
# 16 "##  config: $config"
# 32 "Generating blockages for TrionRoute"
# 34 "Inserting macro grid for instance $instance"
#
# Warning
# 1 "run_pdngen is deprecated. Use pdngen."
# 2 "No shapes on layer $l1 for $tag"
# 3 "No shapes on layer $l2 for $tag"
# 4 "Unexpected number of points in connection shape ($l1,$l2 $tag [llength $points])"
# 5 (points list)
# 6 "Unexpected number of points in shape ($lay $signal_type [llength $points])"
# 7 (points list)
# 17 "No stdcell grid specification found - no rails inserted"
# 18 "No macro grid specifications found - no straps added"
# 35 "No track information found for layer $layer_name"
# 
# Error
# 
# Critical
# 19 "Cannot find layer $layer_name in loaded technology"
# 20 "Failed to read CUTCLASS property '$line'"
# 21 "Failed to read ENCLOSURE property '$line'"
# 22 "Cant find lower metal layer $layer1"
# 23 "Cant find upper metal layer $layer2"
# 24 "Missing key [dict get $intersection rule]\nAvailable keys [dict keys $logical_viarules]"
# 25 "Unexpected row orientation $orient for row [$row getName]"
# 26 "Invalid direction \"[get_dir $layer]\" for metal layer ${layer}. Should be either \"hor\" or \"ver\"."
# 27 "Illegal orientation $orientation specified"
# 28 "File $PDN_cfg does not exist, or exists but empty"
# 29 "Illegal number of elements defined for ::halo \"$::halo\" (1, 2 or 4 allowed)"
# 30 "Layer specified for std. cell rails '$layer' not in list of layers."
# 31 "No matching grid specification found for $instance"
# 33 "Unknown direction for layer $layer_name"
#
# 9999 - Unexpected error
#

proc pdngen { args } {
  sta::parse_key_args "pdngen" args \
    keys {} flags {-verbose}

  set verbose [info exists flags(-verbose)]

  sta::check_argc_eq1 "pdngen" $args
  set config_file $args

  if {[catch {pdngen::apply_pdn $config_file $verbose } error_msg]} {
    if {[regexp {\[PDNG\-[0-9]*\]} $error_msg]} {
      puts $error_msg
    } else {
      pdngen::critical 9999 "Unexpected error: $error_msg"
    }
    error "Execution stopped"
  }
}

# temporary alias to old name
proc run_pdngen { args } {
  pdngen::warning 1 "run_pdngen is deprecated. Use pdngen."
  pdngen $args
}

namespace eval pdngen {

variable logical_viarules {}
variable physical_viarules {}
variable vias {}
variable stripe_locs
variable layers {}
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
variable layers {}
variable blockages {} 
variable instances {}
variable default_template_name {}
variable template {}
variable default_cutclass {}
variable twowidths_table {}
variable twowidths_table_wrongdirection {}

#This file contains procedures that are used for PDN generation
proc set_message {level message} {
  return "\[$level\] $message"
}

proc debug {message} {
  set state [info frame -1]
  set str ""
  if {[dict exists $state file]} {
    set str "$str[dict get $state file]:"
  }
  if {[dict exists $state proc]} {
    set str "$str[dict get $state proc]:"
  }
  if {[dict exists $state line]} {
    set str "$str[dict get $state line]"
  }
  puts [set_message DEBUG "$str: $message"]
}

proc information {id message} {
  puts [set_message INFO [format "\[PDNG-%04d\] %s" $id $message]]
}

proc warning {id message} {
  puts [set_message WARN [format "\[PDNG-%04d\] %s" $id $message]]
}

proc err {id message} {
  puts [set_message ERROR [format "\[PDNG-%04d\] %s" $id $message]]
}

proc critical {id message} {
  error [set_message CRIT [format "\[PDNG-%04d\] %s" $id $message]]
}

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
  variable layers 

  if {[regexp {.*_PIN_(hor|ver)} $layer_name - dir]} {
    return $dir
  }
  
  if {![dict exists $layers $layer_name direction]} {
    critical 33 "Unknown direction for layer $layer_name"
  }
  return [dict get $layers $layer_name direction]
}

proc get_rails_layers {} {
  variable design_data
  
  if {[dict exists $design_data grid]} {
    foreach type [dict keys [dict get $design_data grid]] {
      dict for {name specification} [dict get $design_data grid $type] {
        if {[dict exists $specification rails]} {
          return [dict keys [dict get $specification rails]]
        }
      }
    }
  }
  return {}
}

proc is_rails_layer {layer} {
  return [expr {[lsearch -exact [get_rails_layers] $layer] > -1}]
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

proc set_prop_lines {obj prop_name} {
  variable prop_line
  if {[set prop [::odb::dbStringProperty_find $obj $prop_name]] != "NULL"} {
    set prop_line [$prop getValue]
  } else {
    set prop_line {}
  }
}

proc read_propline {} {
  variable prop_line

  set word [lindex $prop_line 0]
  set prop_line [lrange $prop_line 1 end]

  set line {}
  while {[llength $prop_line] > 0 && $word != ";"} {
    lappend line $word
    set word [lindex $prop_line 0]
    set prop_line [lrange $prop_line 1 end]
  }
  return $line
}

proc empty_propline {} {
  variable prop_line
  return [expr ![llength $prop_line]]
}

proc find_layer {layer_name} {
  variable tech

  if {[set layer [$tech findLayer $layer_name]] == "NULL"} {
    critical 19 "Cannot find layer $layer_name in loaded technology"
  }
  return $layer
}

proc read_spacing {layer_name} {
  variable layers 
  variable def_units

  set layer [find_layer $layer_name]

  set_prop_lines $layer LEF58_SPACING
  set spacing {}

  while {![empty_propline]} {
    set line [read_propline]
    if {[set idx [lsearch -exact $line CUTCLASS]] > -1} {
      set cutclass [lindex $line [expr $idx + 1]]
      set line [lreplace $line $idx [expr $idx + 1]]
      
      if {[set idx [lsearch -exact $line LAYER]] > -1} {
        set other_layer [lindex $line [expr $idx + 1]]
        set line [lreplace $line $idx [expr $idx + 1]]

        if {[set idx [lsearch -exact $line CONCAVECORNER]] > -1} {
          set line [lreplace $line $idx $idx]
          
          if {[set idx [lsearch -exact $line SPACING]] > -1} {
            dict set spacing $cutclass $other_layer concave [expr round([lindex $line [expr $idx + 1]] * $def_units)]
            # set line [lreplace $line $idx [expr $idx + 1]]
          }
        }
      }
    }
  }
  # debug "$layer_name $spacing"
  dict set layers $layer_name spacing $spacing 
  # debug "$layer_name [dict get $layers $layer_name]"
}

proc read_spacingtables {layer_name} {
  variable layers 
  variable def_units

  set layer [find_layer $layer_name]
  set prls {}

  if {[$layer hasTwoWidthsSpacingRules]} {
    set type "TWOWIDTHS"
    set subtype "NONE"
    
    set table_size [$layer getTwoWidthsSpacingTableNumWidths]
    for {set i 0} {$i < $table_size} {incr i} {
      set width [$layer getTwoWidthsSpacingTableWidth $i]

      if {[$layer getTwoWidthsSpacingTableHasPRL $i]} {
        set prl [$layer getTwoWidthsSpacingTablePRL $i] 
      } else {
        set prl 0
      }
      set spacings {}
      for {set j 0} {$j < $table_size} {incr j} {
        lappend spacings [$layer getTwoWidthsSpacingTableEntry $i $j]
      }
      
      dict set layers $layer_name spacingtable $type $subtype $width [list prl $prl spacings $spacings]
    }
  }
  
  set_prop_lines $layer LEF58_SPACINGTABLE
  set spacing {}

  while {![empty_propline]} {
    set line [read_propline]
    # debug "$line"
    set type [lindex $line 1]
    set subtype [lindex $line 2]
    
    set table_entry_indexes [lsearch -exact -all $line "WIDTH"] 
    set num_entries [llength $table_entry_indexes]
    
    foreach start_index $table_entry_indexes {
      set pos $start_index
      incr pos
      set width [expr round([lindex $line $pos] * $def_units)]
      incr pos
      if {[lindex $line $pos] == "PRL"} {
        incr pos
        set prl [expr round([lindex $line $pos] * $def_units)]
        incr pos
      } else {
        set prl 0
      }
      set spacings {}
      for {set i 0} {$i < $num_entries} {incr i} {
        # debug "[expr $i + $pos] [lindex $line [expr $i + $pos]]"
        lappend spacings [expr round([lindex $line [expr $i + $pos]] * $def_units)]
      }
      dict set layers $layer_name spacingtable $type $subtype $width [list prl $prl spacings $spacings]
    }
  }
   
  if {![dict exists $layers $layer_name spacingtable]} {
    dict set layers $layer_name spacingtable {}
  }
  # debug "$layer_name [dict get $layers $layer_name]"
}

proc get_spacingtables {layer_name} {
  variable layers
  
  if {![dict exists $layers $layer_name spacingtable]} {
    read_spacingtables $layer_name
  }
  
  return [dict get $layers $layer_name spacingtable]
}

proc get_concave_spacing_value {layer_name other_layer_name} {
  variable layers
  variable default_cutclass
  
  if {![dict exists $layers $layer_name spacing]} {
    read_spacing $layer_name
  }
  # debug "$layer_name [dict get $layers $layer_name]"
  if {[dict exists $layers $layer_name spacing [dict get $default_cutclass $layer_name] $other_layer_name concave]} {
    return [dict get $layers $layer_name spacing [dict get $default_cutclass $layer_name] $other_layer_name concave]
  }
  return 0
}

proc read_arrayspacing {layer_name} {
  variable layers 
  variable def_units

  set layer [find_layer $layer_name]

  set_prop_lines $layer LEF58_ARRAYSPACING
  set arrayspacing {}
  
  while {![empty_propline]} {
    set line [read_propline]
    if {[set idx [lsearch -exact $line PARALLELOVERLAP]] > -1} {
      dict set arrayspacing paralleloverlap 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line LONGARRAY]] > -1} {
      dict set arrayspacing longarray 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line CUTSPACING]] > -1} {
      dict set arrayspacing cutspacing [expr round([lindex $line [expr $idx + 1]] * $def_units)]
      set line [lreplace $line $idx [expr $idx + 1]]
    }
    while {[set idx [lsearch -exact $line ARRAYCUTS]] > -1} {
      dict set arrayspacing arraycuts [lindex $line [expr $idx + 1]] spacing [expr round([lindex $line [expr $idx + 3]] * $def_units)]
      set line [lreplace $line $idx [expr $idx + 3]]
    }
  }
  dict set layers $layer_name arrayspacing $arrayspacing 
}

proc read_cutclass {layer_name} {
  variable layers 
  variable def_units
  variable default_cutclass

  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_CUTCLASS
  dict set layers $layer_name cutclass {}
  set min_area -1
  
  while {![empty_propline]} {
    set line [read_propline]
    if {![regexp {CUTCLASS\s+([^\s]+)\s+WIDTH\s+([^\s]+)\s+LENGTH\s+([^\s]+)} $line - cut_class width length]} {
      critical 20 "Failed to read CUTCLASS property '$line'"
    }
    if {$min_area == -1 || ($width * $length) < $min_area} {
      dict set default_cutclass $layer_name $cut_class
      set min_area [expr $width * $length]
    }
    dict set layers $layer_name cutclass $cut_class [list width [expr round($width * $def_units)] length [expr round($length * $def_units)]]
  }
}

proc read_enclosures {layer_name} {
  variable layers 
  variable def_units
  
  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_ENCLOSURE
  set prev_cutclass ""

  while {![empty_propline]} {
    set line [read_propline]
    # debug "$line"
    set enclosure {}
    if {[set idx [lsearch -exact $line EOL]] > -1} {
      continue
      dict set enclosure eol [expr round([lindex $line [expr $idx + 1]] * $def_units)]
      set line [lreplace $line $idx [expr $idx + 1]]
    }
    if {[set idx [lsearch -exact $line EOLONLY]] > -1} {
      dict set enclosure eolonly 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line SHORTEDGEONEOL]] > -1} {
      dict set enclosure shortedgeoneol 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line MINLENGTH]] > -1} {
      dict set enclosure minlength [expr round([lindex $line [expr $idx + 1]] * $def_units)]
      set line [lreplace $line $idx [expr $idx + 1]]
    }
    if {[set idx [lsearch -exact $line ABOVE]] > -1} {
      dict set enclosure above 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line BELOW]] > -1} {
      dict set enclosure below 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line END]] > -1} {
      dict set enclosure end 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line SIDE]] > -1} {
      dict set enclosure side 1
      set line [lreplace $line $idx $idx]
    }

    set width 0
    regexp {WIDTH\s+([^\s]+)} $line - width
    set width [expr round($width * $def_units)]
    
    if {![regexp {ENCLOSURE CUTCLASS\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)} $line - cut_class overlap1 overlap2]} {
      critical 21 "Failed to read ENCLOSURE property '$line'"
    }
    dict set enclosure overlap1 [expr round($overlap1 * $def_units)]
    dict set enclosure overlap2 [expr round($overlap2 * $def_units)]
    # debug "class - $cut_class enclosure - $enclosure"
    if {$prev_cutclass != $cut_class} {
      set enclosures {}
      set prev_cutclass $cut_class
    }
    dict lappend enclosures $width $enclosure
    dict set layers $layer_name cutclass $cut_class enclosures $enclosures 
  }
  # debug "end"
}

proc get_via_enclosure {via_info lower_width upper_width} {
  variable layers
  variable default_cutclass
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  
  # debug "via_info $via_info width $lower_width,$upper_width"
  set layer_name [dict get $via_info cut layer]
  
  if {![dict exists $layers $layer_name cutclass]} {
    read_cutclass $layer_name
    read_enclosures $layer_name
  }

  if {!([dict exists $default_cutclass $layer_name] && [dict exists $layers $layer_name cutclass [dict get $default_cutclass $layer_name] enclosures])} {
    set lower_enclosure [dict get $via_info lower enclosure]
    set upper_enclosure [dict get $via_info upper enclosure]

    set min_lower_enclosure [lindex $lower_enclosure 0]
    set max_lower_enclosure [lindex $lower_enclosure 1]

    if {$max_lower_enclosure < $min_lower_enclosure} {
      set swap $min_lower_enclosure
      set min_lower_enclosure $max_lower_enclosure
      set max_lower_enclosure $swap
    }

    set min_upper_enclosure [lindex $upper_enclosure 0]
    set max_upper_enclosure [lindex $upper_enclosure 1]

    if {$max_upper_enclosure < $min_upper_enclosure} {
      set swap $min_upper_enclosure
      set min_upper_enclosure $max_upper_enclosure
      set max_upper_enclosure $swap
    }
    
    set selected_enclosure [list $min_lower_enclosure $max_lower_enclosure $min_upper_enclosure $max_upper_enclosure]
  } else {
    set enclosures [dict get $layers $layer_name cutclass [dict get $default_cutclass $layer_name] enclosures]
    # debug "Enclosure set $enclosures"
    set upper_enclosures {}
    set lower_enclosures {}
    
    set width $lower_width
      
    foreach size [lreverse [dict keys $enclosures]] {
      if {$width >= $size} {
          break
      }
    }

    set enclosure_list [dict get $enclosures $size]
    # debug "Initial enclosure_list (size = $size)- $enclosure_list"
    if {$size > 0} {
      foreach enclosure $enclosure_list {
        if {![dict exists $enclosure above]} {
          lappend lower_enclosures $enclosure
        }
      }
    }

    set width $upper_width
      
    foreach size [lreverse [dict keys $enclosures]] {
      if {$width >= $size} {
          break
      }
    }

    set enclosure_list [dict get $enclosures $size]
    # debug "Initial enclosure_list (size = $size)- $enclosure_list"
    if {$size > 0} {
      foreach enclosure $enclosure_list {
        if {![dict exists $enclosure above]} {
          lappend upper_enclosures $enclosure
        }
      }
    }

    if {[llength $upper_enclosures] == 0} {
      set zero_enclosures_list [dict get $enclosures 0]
      foreach enclosure $zero_enclosures_list {
        if {![dict exists $enclosure below]} {
          lappend upper_enclosures $enclosure
        }
      }
    }
    if {[llength $lower_enclosures] == 0} {
      set zero_enclosures_list [dict get $enclosures 0]
      foreach enclosure $zero_enclosures_list {
        if {![dict exists $enclosure above]} {
          lappend lower_enclosures $enclosure
        }
      }
    }
    set upper_min -1
    set lower_min -1
    if {[llength $upper_enclosures] > 1} {
      foreach enclosure $upper_enclosures {
        # debug "upper enclosure - $enclosure"
        set this_min [expr min([dict get $enclosure overlap1], [dict get $enclosure overlap2])]
        if {$upper_min < 0 || $this_min < $upper_min} {
          set upper_min $this_min
          set upper_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
          # debug "upper_enc: $upper_enc"
        }
      }
    } else {
      set enclosure [lindex $upper_enclosures 0]
      set upper_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
    }
    if {[llength $lower_enclosures] > 1} {
      foreach enclosure $lower_enclosures {
        # debug "lower enclosure - $enclosure"
        set this_min [expr min([dict get $enclosure overlap1], [dict get $enclosure overlap2])]
        if {$lower_min < 0 || $this_min < $lower_min} {
          set lower_min $this_min
          set lower_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
        }
      }
      # debug "[llength $lower_enclosures] lower_enc: $lower_enc"
    } else {
      set enclosure [lindex $lower_enclosures 0]
      set lower_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
      # debug "1 lower_enc: lower_enc: $lower_enc"
    }
    set selected_enclosure [list {*}$lower_enc {*}$upper_enc]
  }
  # debug "selected $selected_enclosure"
  set min_lower_enclosure [expr min([lindex $selected_enclosure 0], [lindex $selected_enclosure 1])]
  set max_lower_enclosure [expr max([lindex $selected_enclosure 0], [lindex $selected_enclosure 1])]
  set min_upper_enclosure [expr min([lindex $selected_enclosure 2], [lindex $selected_enclosure 3])]
  set max_upper_enclosure [expr max([lindex $selected_enclosure 2], [lindex $selected_enclosure 3])]
  # debug "enclosures - min_lower $min_lower_enclosure max_lower $max_lower_enclosure min_upper $min_upper_enclosure max_upper $max_upper_enclosure"
}

proc select_via_info {lower} {
  variable def_via_tech

  set layer_name $lower
  regexp {(.*)_PIN} $lower - layer_name
  
  return [dict filter $def_via_tech script {rule_name rule} {expr {[dict get $rule lower layer] == $layer_name}}]
}

proc set_layer_info {layer_info} {
  variable layers
  
  set layers $layer_info
}

proc read_widthtable {layer_name} {
  variable tech
  variable def_units
  
  set table {}
  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_WIDTHTABLE

  while {![empty_propline]} {
    set line [read_propline]
    set flags {}
    if {[set idx [lsearch -exact $line ORTHOGONAL]] > -1} {
      dict set flags orthogonal 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line WRONGDIRECTION]] > -1} {
      dict set flags wrongdirection 1
      set line [lreplace $line $idx $idx]
    }

    regexp {WIDTHTABLE\s+(.*)} $line - widthtable
    set widthtable [lmap x $widthtable {expr round($x * $def_units)}]
    
    # Not interested in wrong direction routing
    if {![dict exists $flags wrongdirection]} {
      set table $widthtable
    }
  }
  return $table
}

proc get_widthtable {layer_name} {
  variable layers
  
  if {![dict exists $layers $layer_name widthtable]} {
      dict set layers $layer_name widthtable [read_widthtable $layer_name]
  }

  return [dict get $layers $layer_name widthtable]
}

# Layers that have a widthtable will only support some width values, the widthtable defines the 
# set of widths that are allowed, or any width greater than or equal to the last value in the
# table
proc get_adjusted_width {layer width} {
  set widthtable [get_widthtable $layer]

  if {[llength $widthtable] == 0} {return $width}
  # debug "widthtable $layer ($width): $widthtable"
  if {[lsearch -exact $widthtable $width] > -1} {return $width}
  if {$width > [lindex $widthtable end]} {return $width}

  foreach value $widthtable {
    if {$value > $width} {
      return $value
    }
  }
  
  return $width
}

proc get_arrayspacing_rule {layer_name} {
  variable layers
  
  if {![dict exists $layers $layer_name arrayspacing]} {
    read_arrayspacing $layer_name
  }
  
  return [dict get $layers $layer_name arrayspacing]
}

proc use_arrayspacing {layer_name rows columns} {
  set arrayspacing [get_arrayspacing_rule $layer_name]
  # debug "$arrayspacing"
  # debug "$rows $columns"
  if {[llength $arrayspacing] == 0} {
    # debug "No array spacing rule defined"
    return 0
  }
  # debug "[dict keys [dict get $arrayspacing arraycuts]]"
  if {[dict exists $arrayspacing arraycuts [expr min($rows,$columns)]]} {
    # debug "Matching entry in arrayspacing"
    return 1
  }
  if {min($rows,$columns) < [lindex [dict keys [dict get $arrayspacing arraycuts]] 0]} {
    # debug "row/columns less than min array spacing"
    return 0
  }
  if {min($rows,$columns) > [lindex [dict keys [dict get $arrayspacing arraycuts]] end]} {
    # debug "row/columns greater than min array spacing"
    return 1
  }
  # debug "default 1"
  return 1
}

proc determine_num_via_columns {via_info constraints} {
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable cut_width
  variable xcut_pitch
  variable xcut_spacing
  variable def_units

  # What are the maximum number of columns that we can fit in this space?
  set i 1
  if {$lower_dir == "hor"} {
    set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
    set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
  } else {
    set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
    set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
  }
  if {[dict exists $constraints cut_pitch]} {set xcut_pitch [expr round([dict get $constraints cut_pitch] * $def_units)]}
  
  while {$via_width_lower < $lower_width && $via_width_upper < $upper_width} {
    incr i
    if {$lower_dir == "hor"} {
      set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
      set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
    } else {
      set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
      set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
    }
  }
  set xcut_spacing [expr $xcut_pitch - $cut_width]
  set columns [expr max(1, $i - 1)]
  # debug "cols $columns W: via_width_lower $via_width_lower >= lower_width $lower_width || via_width_upper $via_width_upper >= upper_width $upper_width"
  if {[dict exists $constraints max_columns]} {
    if {$columns > [dict get $constraints max_columns]} {
      set columns [dict get $constraints max_columns]

      set lower_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info lower layer]]
      # debug "$lower_concave_enclosure $max_lower_enclosure"
      if {$lower_concave_enclosure > $max_lower_enclosure} {
        set max_lower_enclosure $lower_concave_enclosure
      }
      set upper_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info upper layer]]
      # debug "$upper_concave_enclosure $max_upper_enclosure"
      if {$upper_concave_enclosure > $max_upper_enclosure} {
        set max_upper_enclosure $upper_concave_enclosure
      }
    }

  }
  # debug "$lower_dir"
  if {$lower_dir == "hor"} {
    if {[dict get $constraints stack_top] != [dict get $via_info upper layer]} {
      get_via_enclosure $via_info [expr min($lower_width,$lower_height)] [expr min([expr $cut_width + $xcut_pitch * ($columns - 1)],$upper_height)]
      set upper_width [expr $cut_width + $xcut_pitch * ($columns - 1) + 2 * $min_upper_enclosure]
    }
  } else {
    get_via_enclosure $via_info [expr min([expr $cut_width + $xcut_pitch * ($columns - 1)],$lower_height)] [expr min($upper_width,$upper_height)]
    set lower_width [expr $cut_width + $xcut_pitch * ($columns - 1) + 2 * $min_lower_enclosure]
  }
  # debug "cols $columns W: lower $lower_width upper $upper_width"
  set lower_width [get_adjusted_width [dict get $via_info lower layer] $lower_width]
  set upper_width [get_adjusted_width [dict get $via_info upper layer] $upper_width]
  # debug "cols $columns W: lower $lower_width upper $upper_width"

  return $columns
}

proc determine_num_via_rows {via_info constraints} {
  variable cut_height
  variable ycut_pitch
  variable ycut_spacing
  variable upper_height
  variable lower_height
  variable lower_width
  variable upper_width
  variable lower_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable def_units
  
  # What are the maximum number of rows that we can fit in this space?
  set i 1
  if {$lower_dir == "hor"} {
    set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
    set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
  } else {
    set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
    set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
  }
  if {[dict exists $constraints cut_pitch]} {set ycut_pitch [expr round([dict get $constraints cut_pitch] * $def_units)]}
  while {$via_height_lower < $lower_height || $via_height_upper < $upper_height} {
    incr i
    if {$lower_dir == "hor"} {
      set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
      set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
    } else {
      set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
      set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
    }
  }
  set ycut_spacing [expr $ycut_pitch - $cut_height]
  set rows [expr max(1,$i - 1)]
  # debug "$rows H: $via_height_lower >= $lower_height && $via_height_upper >= $upper_height"
  if {[dict exists $constraints max_rows]} {
    if {$rows > [dict get $constraints max_rows]} {
      set rows [dict get $constraints max_rows]

      set lower_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info lower layer]]
      # debug "$lower_concave_enclosure $max_lower_enclosure"
      if {$lower_concave_enclosure > $max_lower_enclosure} {
        set max_lower_enclosure $lower_concave_enclosure
      }
      set upper_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info upper layer]]
      # debug "$upper_concave_enclosure $max_upper_enclosure"
      if {$upper_concave_enclosure > $max_upper_enclosure} {
        set max_upper_enclosure $upper_concave_enclosure
      }

    }                                                                              
  }
  if {$lower_dir == "hor"} {
    get_via_enclosure $via_info [expr min($lower_width,[expr $cut_height + $ycut_pitch * ($rows - 1)])] [expr min($upper_width,$upper_height)]
    set lower_height [expr $cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_lower_enclosure]
    # debug "modify lower_height to $lower_height ($cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_lower_enclosure"
  } else {
    # debug "[dict get $constraints stack_top] != [dict get $via_info upper layer]"
    if {[dict get $constraints stack_top] != [dict get $via_info upper layer]} {
      get_via_enclosure $via_info [expr min($lower_width,$lower_height)] [expr min($upper_width,[expr $cut_height + $ycut_pitch * ($rows - 1)])]
      set upper_height [expr $cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_upper_enclosure]
      # debug "modify upper_height to $upper_height ($cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_upper_enclosure"
    }
  }
  # debug "$rows H: lower $lower_height upper $upper_height"
  set lower_height [get_adjusted_width [dict get $via_info lower layer] $lower_height]
  set upper_height [get_adjusted_width [dict get $via_info upper layer] $upper_height]
  # debug "$rows H: lower $lower_height upper $upper_height"


  return $rows
}

proc init_via_width_height {lower_layer width height constraints} {
  variable def_units
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable cut_width
  variable cut_height
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing

  set via_info [lindex [select_via_info $lower_layer] 1]
  set upper_layer [dict get $via_info upper layer]

  set xcut_pitch [lindex [dict get $via_info cut spacing] 0]
  set ycut_pitch [lindex [dict get $via_info cut spacing] 0]

  set cut_width   [lindex [dict get $via_info cut size] 0]
  set cut_height  [lindex [dict get $via_info cut size] 1]
  
  if {[dict exists $constraints split_cuts $lower_layer]} {
    if {[get_dir $lower_layer] == "hor"} {
      set ycut_pitch [expr round([dict get $constraints split_cuts $lower_layer] * $def_units)]
    } else {
      set xcut_pitch [expr round([dict get $constraints split_cuts $lower_layer] * $def_units)]
    }
  } 

  if {[dict exists $constraints split_cuts $upper_layer]} {
    if {[get_dir $upper_layer] == "hor"} {
      set ycut_pitch [expr round([dict get $constraints split_cuts $upper_layer] * $def_units)]
    } else {
      set xcut_pitch [expr round([dict get $constraints split_cuts $upper_layer] * $def_units)]
    }
  } 

  if {[dict exists $constraints width $lower_layer]} {
    if {[get_dir $lower_layer] == "hor"} {
      set lower_height [expr round([dict get $constraints width $lower_layer] * $def_units)]
      set lower_width  [get_adjusted_width $lower_layer $width]
    } else {
      set lower_width [expr round([dict get $constraints width $lower_layer] * $def_units)]
      set lower_height [get_adjusted_width $lower_layer $height]
    }
  } else {
    # Adjust the width and height values to the next largest allowed value if necessary
    set lower_width  [get_adjusted_width $lower_layer $width]
    set lower_height [get_adjusted_width $lower_layer $height]
  }
  if {[dict exists $constraints width $upper_layer]} {
    if {[get_dir $upper_layer] == "hor"} {
      set upper_height [expr round([dict get $constraints width $upper_layer] * $def_units)]
      set upper_width  [get_adjusted_width $upper_layer $width]
    } else {
      set upper_width [expr round([dict get $constraints width $upper_layer] * $def_units)]
      set upper_height [get_adjusted_width $upper_layer $height]
    }
  } else {
    set upper_width  [get_adjusted_width $upper_layer $width]
    set upper_height [get_adjusted_width $upper_layer $height]
  }
  # debug "lower (width $lower_width height $lower_height) upper (width $upper_width height $upper_height)"
  # debug "min - \[expr min($lower_width,$lower_height,$upper_width,$upper_height)\]"
}

proc get_enclosure_by_direction {layer xenc yenc max_enclosure min_enclosure} {
  set info {}
  if {$xenc > $max_enclosure && $yenc > $min_enclosure || $xenc > $min_enclosure && $yenc > $max_enclosure} {
    # If the current enclosure values meet the min/max enclosure requirements either way round, then keep
    # the current enclsoure settings
    dict set info xEnclosure $xenc
    dict set info yEnclosure $yenc
  } else {
    # Enforce min/max enclosure rule, with max_enclosure along the preferred direction of the layer.
    if {[get_dir $layer] == "hor"} {
      dict set info xEnclosure [expr max($xenc,$max_enclosure)]
      dict set info yEnclosure [expr max($yenc,$min_enclosure)]
    } else {
      dict set info xEnclosure [expr max($xenc,$min_enclosure)]
      dict set info yEnclosure [expr max($yenc,$max_enclosure)]
    }
  }
  
  return $info
}

proc via_generate_rule {rule_name rows columns constraints} {
  variable via_info
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing
  variable cut_height
  variable cut_width
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure

  set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]
  set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]
  
  set lower [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
  set upper [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]
  # debug "rule $rule_name"
  # debug "lower: width $lower_width height $lower_height"
  # debug "lower: enc_width $lower_enc_width enc_height $lower_enc_height enclosure_rule $max_lower_enclosure $min_lower_enclosure"
  # debug "lower: enclosure [dict get $lower xEnclosure] [dict get $lower yEnclosure]"

  return [list [list \
    name $rule_name \
    rule [lindex [select_via_info [dict get $via_info lower layer]] 0] \
    cutsize [dict get $via_info cut size] \
    layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] [dict get $via_info upper layer]] \
    cutspacing [list $xcut_spacing $ycut_spacing] \
    rowcol [list $rows $columns] \
    lower_rect [list [expr -1 * $lower_width / 2] [expr -1 * $lower_height / 2] [expr $lower_width / 2] [expr $lower_height / 2]] \
    upper_rect [list [expr -1 * $upper_width / 2] [expr -1 * $upper_height / 2] [expr $upper_width / 2] [expr $upper_height / 2]] \
    enclosure [list \
      [dict get $lower xEnclosure] \
      [dict get $lower yEnclosure] \
      [dict get $upper xEnclosure] \
      [dict get $upper yEnclosure] \
    ] \
    origin_x 0 origin_y 0
  ]]
}

proc via_generate_array_rule {rule_name rows columns} {
  variable via_info
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing
  variable cut_height
  variable cut_width
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure

  # We need array vias -
  # if the min(rows,columns) > ARRAYCUTS
  #   determine which direction gives best number of CUTs wide using min(ARRAYCUTS)
  #   After adding ARRAYs, is there space for more vias
  #   Add vias to the rule with appropriate origin setting
  # else
  #   add a single via with min(rows,columns) cuts - hor/ver as required

  
  set spacing_rule [get_arrayspacing_rule [dict get $via_info cut layer]]
  set array_size [expr min($rows, $columns)]

  set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]
  set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]

  if {$array_size > [lindex [dict keys [dict get $spacing_rule arraycuts]] end]} {
    # debug "Multi-viaArrayspacing rule"
    set use_array_size [lindex [dict keys [dict get $spacing_rule arraycuts]] 0]
    foreach other_array_size [lrange [dict keys [dict get $spacing_rule arraycuts]] 1 end] {
      if {$array_size % $use_array_size > $array_size % $other_array_size} {
        set use_array_size $other_array_size
      }
    }
    set num_arrays [expr $array_size / $use_array_size]
    set array_spacing [expr max($xcut_spacing,$ycut_spacing,[dict get $spacing_rule arraycuts $use_array_size spacing])]

    set rule [list \
      rule [lindex [select_via_info [dict get $via_info lower layer]] 0] \
      cutsize [dict get $via_info cut size] \
      layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] [dict get $via_info upper layer]] \
      cutspacing [list $xcut_spacing $ycut_spacing] \
      origin_x 0 \
      origin_y 0 \
    ]
    # debug "$rule"
    set rule_list {}
    if {$array_size == $rows} {
      # Split into num_arrays rows of arrays
      set array_min_size [expr [lindex [dict get $via_info cut size] 0] * $use_array_size + [dict get $spacing_rule cutspacing] * ($use_array_size - 1)]
      set total_array_size [expr $array_min_size * $num_arrays + $array_spacing * ($num_arrays - 1)]
      # debug "Split into $num_arrays rows of arrays"

      set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($use_array_size - 1))) / 2)]
      set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($use_array_size - 1))) / 2)]

      set lower_enc [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
      set upper_enc [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]

      dict set rule rowcol [list $use_array_size $columns]
      dict set rule name "[dict get $via_info cut layer]_ARRAY_${use_array_size}X${columns}"
      dict set rule enclosure [list \
        [dict get $lower_enc xEnclosure] \
        [dict get $lower_enc yEnclosure] \
        [dict get $upper_enc xEnclosure] \
        [dict get $upper_enc yEnclosure] \
      ]

      set y [expr $array_min_size / 2 - $total_array_size / 2]
      for {set i 0} {$i < $num_arrays} {incr i} {
        dict set rule origin_y $y
        lappend rule_list $rule
        set y [expr $y + $array_spacing + $array_min_size]
      }
    } else {
      # Split into num_arrays columns of arrays
      set array_min_size [expr [lindex [dict get $via_info cut size] 1] * $use_array_size + [dict get $spacing_rule cutspacing] * ($use_array_size - 1)]
      set total_array_size [expr $array_min_size * $num_arrays + $array_spacing * ($num_arrays - 1)]
      # debug "Split into $num_arrays columns of arrays"

      set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($use_array_size - 1))) / 2)]
      set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($use_array_size - 1))) / 2)]

      set lower_enc [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
      set upper_enc [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]

      dict set rule rowcol [list $rows $use_array_size]
      dict set rule name "[dict get $via_info cut layer]_ARRAY_${rows}X${use_array_size}"
      dict set rule enclosure [list \
        [dict get $lower_enc xEnclosure] \
        [dict get $lower_enc yEnclosure] \
        [dict get $upper_enc xEnclosure] \
        [dict get $upper_enc yEnclosure] \
      ]

      set x [expr $array_min_size / 2 - $total_array_size / 2]
      for {set i 0} {$i < $num_arrays} {incr i} {
        dict set rule origin_x $x
        lappend rule_list $rule
        set x [expr $x + $array_spacing + $array_min_size]
      }
    }
  } else {
    # debug "Arrayspacing rule"
    set lower_enc [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
    set upper_enc [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]

    set rule [list \
      name $rule_name \
      rule [lindex [select_via_info [dict get $via_info lower layer]] 0] \
      cutsize [dict get $via_info cut size] \
      layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] [dict get $via_info upper layer]] \
      cutspacing [list $xcut_spacing $ycut_spacing] \
      rowcol [list $rows $columns] \
      enclosure [list \
        [dict get $lower_enc xEnclosure] \
        [dict get $lower_enc yEnclosure] \
        [dict get $upper_enc xEnclosure] \
        [dict get $upper_enc yEnclosure] \
      ] \
      origin_x 0 \
      origin_y 0 \
    ]
    set rule_list [list $rule]
  }
  
  return $rule_list
}

proc via_split_cuts_rule {rows columns constraints} {
  variable tech
  variable def_units
  variable via_info
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable cut_width
  variable cut_height
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing

  set lower_rects {}
  set cut_rects   {}
  set upper_rects {}

  set lower [dict get $via_info lower layer]
  set upper [dict get $via_info upper layer]
  # debug $via_info
  # debug "lower $lower upper $upper"
  
  set rule {}
  set rule [list \
    rule [lindex [select_via_info $lower] 0] \
    cutsize [dict get $via_info cut size] \
    layers [list $lower [dict get $via_info cut layer] $upper] \
    cutspacing [list $xcut_spacing $ycut_spacing] \
    rowcol [list 1 1] \
  ]

  # Enclosure was calculated from full width of intersection - need to recalculate for min cut size.
  get_via_enclosure $via_info 0 0

  # Area is stored in real units, adjust to def_units
  set lower_area [expr round([[find_layer $lower] getArea] * $def_units * $def_units)]
  set upper_area [expr round([[find_layer $upper] getArea] * $def_units * $def_units)]

  if {[get_dir $lower] == "hor"} {
    if {[dict exists $constraints split_cuts $lower]} {
      set lower_height [expr $cut_height + $min_lower_enclosure]
      set min_lower_length [expr $lower_area / $lower_height]
      if {$min_lower_length % 2 == 1} {incr min_lower_length}
      set max_lower_enclosure [expr max(($min_lower_length - $cut_width) / 2, $max_lower_enclosure)]
    }
    
    if {[dict exists $constraints split_cuts $upper]} {
      set upper_width [expr $cut_width + $min_upper_enclosure]
      set min_upper_length [expr $upper_area / $upper_width]
      if {$min_upper_length % 2 == 1} {incr min_upper_length}
      set max_upper_enclosure [expr max(($min_upper_length - $cut_height) / 2, $max_upper_enclosure)]
    }
    
    set width [expr $max_lower_enclosure * 2 + $cut_width]
    set height [expr $max_upper_enclosure * 2 + $cut_width]

    dict set rule name [get_viarule_name $lower $width $height]
    dict set rule enclosure [list $max_lower_enclosure $min_lower_enclosure $min_upper_enclosure $max_upper_enclosure]
  } else {
    if {[dict exists $constraints split_cuts $lower]} {
      set lower_width [expr $cut_width + $min_lower_enclosure]
      set min_lower_length [expr $lower_area / $lower_width]
      if {$min_lower_length % 2 == 1} {incr min_lower_length}
      set max_lower_enclosure [expr max(($min_lower_length - $cut_width) / 2, $max_lower_enclosure)]
    }
    
    if {[dict exists $constraints split_cuts $upper]} {
      set upper_height [expr $cut_height + $min_upper_enclosure]
      set min_upper_length [expr $upper_area / $upper_height]
      if {$min_upper_length % 2 == 1} {incr min_upper_length}
      set max_upper_enclosure [expr max(($min_upper_length - $cut_height) / 2, $max_upper_enclosure)]
    }
    
    set width [expr $max_upper_enclosure * 2 + $cut_width]
    set height [expr $max_lower_enclosure * 2 + $cut_width]

    dict set rule name [get_viarule_name $lower $width $height]
    dict set rule enclosure [list $min_lower_enclosure $max_lower_enclosure $max_upper_enclosure $min_upper_enclosure]
  }
  # debug "min_lower_enclosure $min_lower_enclosure"
  # debug "lower $lower upper $upper enclosure [dict get $rule enclosure]"

  for {set i 0} {$i < $rows} {incr i} {
    for {set j 0} {$j < $columns} {incr j} {
      set centre_x [expr round(($j - (($columns - 1) / 2.0)) * $xcut_pitch)]
      set centre_y [expr round(($i - (($rows - 1)    / 2.0)) * $ycut_pitch)]
      
      dict set rule origin_x $centre_x
      dict set rule origin_y $centre_y
      lappend rule_list $rule
    }
  }
  # debug "split into [llength $rule_list] vias"
  return $rule_list
}

# Given the via rule expressed in via_info, what is the via with the largest cut area that we can make
# Try using a via generate rule
proc get_via_option {lower width height constraints} {
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable via_info
  

  # debug "get_via_option: {$lower $width $height}"
  set via_info [lindex [select_via_info $lower] 1]
  set lower_dir [get_dir $lower]

  set upper [dict get $via_info upper layer]

  init_via_width_height $lower $width $height $constraints
  get_via_enclosure $via_info [expr min($lower_width,$lower_height)] [expr min($upper_width,$upper_height)]

  # debug "split cuts? [dict exists $constraints split_cuts]"
  # debug "lower $lower upper $upper"
  
  # Determines the maximum number of rows and columns that can fit into this width/height
  set columns [determine_num_via_columns $via_info $constraints]
  set rows    [determine_num_via_rows    $via_info $constraints]

  # debug "lower_width $lower_width min_lower_enclosure $min_lower_enclosure"
  # debug "upper_width $upper_width min_upper_enclosure $min_upper_enclosure"
  
  if {[dict exists $constraints split_cuts] && ([lsearch -exact [dict get $constraints split_cuts] $lower] > -1 || [lsearch -exact [dict get $constraints split_cuts] $upper] > -1)} {
    # debug "via_split_cuts_rule"
    set rules [via_split_cuts_rule $rows $columns $constraints]
  } elseif {[use_arrayspacing [dict get $via_info cut layer] $rows $columns]} {
    # debug "via_generate_array_rule"
    set rules [via_generate_array_rule [get_viarule_name $lower $width $height] $rows $columns]
  } else {
    # debug "via_generate_rule"
    set rules [via_generate_rule [get_viarule_name $lower $width $height] $rows $columns $constraints]
  }
  
  return $rules
}

proc get_viarule_name {lower width height} {
  set rules [select_via_info $lower]
  set first_key [lindex [dict keys $rules] 0]
  #if {![dict exists $rules $first_key cut layer]} {
  #  debug "$lower $width $height"
  #  debug "$rules"
  #  debug "$first_key"
  #}
  set cut_layer [dict get $rules $first_key cut layer]

  return ${cut_layer}_${width}x${height}
}

proc get_cut_area {rule} {
  set area 0
  foreach via $rule {
    set area [expr [lindex [dict get $via rowcol] 0] * [lindex [dict get $via rowcol] 0] * [lindex [dict get $via cutsize] 0] * [lindex [dict get $via cutsize] 1]]
  }
  return $area
}

proc select_rule {rule1 rule2} {
  if {[get_cut_area $rule2] > [get_cut_area $rule1]} {
    return $rule2
  }
  return $rule1
}

proc get_via {lower width height constraints} {
  # First cur will assume that all crossing points (x y) are on grid for both lower and upper layers
  # TODO: Refine the algorithm to cope with offgrid intersection points
  variable physical_viarules

  set rule_name [get_viarule_name $lower $width $height]
  
  if {![dict exists $physical_viarules $rule_name]} {
    set selected_rule {}

    dict for {name rule} [select_via_info $lower] {
      set result [get_via_option $lower $width $height $constraints]
      if {$selected_rule == {}} {
        set selected_rule $result
      } else {
        # Choose the best between selected rule and current result, the winner becomes the new selected rule
        set selected_rule [select_rule $selected_rule $result]
      }
    }
    dict set physical_viarules $rule_name $selected_rule
    # debug "Via [dict size $physical_viarules]: $rule_name"
  }

  return $rule_name
}

proc instantiate_via {physical_via_name x y constraints} {
  variable physical_viarules
  variable block
  variable layers
  
  set via_insts {}

  foreach via [dict get $physical_viarules $physical_via_name] {
    # debug "via x $x y $y $via"
    set x_location [expr $x + [dict get $via origin_x]]
    set y_location [expr $y + [dict get $via origin_y]]

    set lower_layer_name [lindex [dict get $via layers] 0]
    set upper_layer_name [lindex [dict get $via layers] 2]

    if {[dict exists $constraints ongrid]} {
      if {[lsearch -exact [dict get $constraints ongrid] $lower_layer_name] > -1} {
        if {[get_dir $lower_layer_name] == "hor"} {
          set y_pitch [dict get $layers $lower_layer_name pitch]
          set y_offset [dict get $layers $lower_layer_name offsetY]

          set y_location [expr ($y - $y_offset + $y_pitch / 2) / $y_pitch * $y_pitch + $y_offset + [dict get $via origin_y]]
        } else {
          set x_pitch [dict get $layers $lower_layer_name pitch]
          set x_offset [dict get $layers $lower_layer_name offsetX]
          
          set x_location [expr ($x - $x_offset + $x_pitch / 2) / $x_pitch * $x_pitch + $x_offset + [dict get $via origin_x]]
        }
      }
      if {[lsearch -exact [dict get $constraints ongrid] $upper_layer_name] > -1} {
        if {[get_dir $lower_layer_name] == "hor"} {
          set x_pitch [dict get $layers $upper_layer_name pitch]
          set x_offset [dict get $layers $upper_layer_name offsetX]
          
          set x_location [expr ($x - $x_offset + $x_pitch / 2) / $x_pitch * $x_pitch + $x_offset + [dict get $via origin_x]]
        } else {
          set y_pitch [dict get $layers $upper_layer_name pitch]
          set y_offset [dict get $layers $upper_layer_name offsetY]

          set y_location [expr ($y - $y_offset + $y_pitch / 2) / $y_pitch * $y_pitch + $y_offset + [dict get $via origin_y]]
        }
      }
    }
    # debug "x: $x -> $x_location"
    # debug "y: $y -> $y_location"

    dict set via x $x_location
    dict set via y $y_location
    
    lappend via_insts $via
  }
  return $via_insts
}

proc generate_vias {layer1 layer2 intersections constraints} {
  variable logical_viarules
  variable metal_layers

  set vias {}
  set layer1_name $layer1
  set layer2_name $layer2
  regexp {(.*)_PIN_(hor|ver)} $layer1 - layer1_name layer1_direction
  
  set i1 [lsearch -exact $metal_layers $layer1_name]
  set i2 [lsearch -exact $metal_layers $layer2_name]
  if {$i1 == -1} {critical 21 "Cant find lower metal layer $layer1"}
  if {$i2 == -1} {critical 22 "Cant find upper metal layer $layer2"}

  # For each layer between l1 and l2, add vias at the intersection
  # debug "  # Intersections [llength $intersections]"
  set count 0
  foreach intersection $intersections {
    if {![dict exists $logical_viarules [dict get $intersection rule]]} {
      critical 24 "Missing key [dict get $intersection rule]\nAvailable keys [dict keys $logical_viarules]"
    }
    set logical_rule [dict get $logical_viarules [dict get $intersection rule]]

    set x [dict get $intersection x]
    set y [dict get $intersection y]
    set width  [dict get $logical_rule width]
    set height  [dict get $logical_rule height]
    
    set connection_layers [lrange $metal_layers $i1 [expr $i2 - 1]]
    # debug "  # Connection layers: [llength $connection_layers]"
    # debug "  Connection layers: $connection_layers"
    dict set constraints stack_top $layer2_name
    foreach lay $connection_layers {
      set via_name [get_via $lay $width $height $constraints]
      foreach via [instantiate_via $via_name $x $y $constraints] {
        lappend vias $via
      }
    }
    
    incr count
    #if {$count % 1000 == 0} {
    #  debug "  # $count / [llength $intersections]"
    #}
  }
  
  return $vias
}

proc get_layers_from_to {from to} {
  variable metal_layers

  set layers {}
  for {set i [lsearch -exact $metal_layers $from]} {$i <= [lsearch -exact $metal_layers $to]} {incr i} {
    lappend layers [lindex $metal_layers $i]
  }
  return $layers
}

## Proc to generate via locations, both for a normal via and stacked via
proc generate_via_stacks {l1 l2 tag constraints} {
  variable logical_viarules
  variable stripe_locs
  variable def_units
  variable grid_data
  
  set area [dict get $grid_data area]
  
  #this variable contains locations of intersecting points of two orthogonal metal layers, between which via needs to be inserted
  #for every intersection. Here l1 and l2 are layer names, and i1 and i2 and their indices, tag represents domain (power or ground)   
  set intersections ""
  #check if layer pair is orthogonal, case 1
  set layer1 $l1
  regexp {(.*)_PIN_(hor|ver)} $l1 - layer1 direction
  
  set layer2 $l2
  
  set ignore_count 0
  if {[array names stripe_locs "$l1,$tag"] == ""} {
    warning 2 "No shapes on layer $l1 for $tag"
    return {}
  }
  if {[array names stripe_locs "$l2,$tag"] == ""} {
    warning 3 "No shapes on layer $l2 for $tag"
    return {}
  }
  set intersection [odb::andSet $stripe_locs($l1,$tag) $stripe_locs($l2,$tag)]

  foreach shape [::odb::getPolygons $intersection] {
    set points [::odb::getPoints $shape]
    if {[llength $points] != 4} {
        variable def_units
        warning 4 "Unexpected number of points in connection shape ($l1,$l2 $tag [llength $points])"
        set str "    "
        foreach point $points {set str "$str ([expr 1.0 * [$point getX] / $def_units ] [expr 1.0 * [$point getY] / $def_units]) "}
        warning 5 $str
        continue
    }
    set xMin [expr min([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set xMax [expr max([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set yMin [expr min([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]
    set yMax [expr max([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]

    set width [expr $xMax - $xMin]
    set height [expr $yMax - $yMin]

    set rule_name ${l1}${layer2}_${width}x${height}
    if {![dict exists $logical_viarules $rule_name]} {
      dict set logical_viarules $rule_name [list lower $l1 upper $layer2 width $width height $height]
    }
    lappend intersections "rule $rule_name x [expr ($xMax + $xMin) / 2] y [expr ($yMax + $yMin) / 2]"
  }
  
  # debug generate_via_stacks "Added [llength $intersections] intersections"

  return [generate_vias $l1 $l2 $intersections $constraints]
}

proc add_stripe {layer type polygon_set} {
  variable stripes
  # debug "start"
  lappend stripes($layer,$type) $polygon_set
  # debug "end"
}

proc merge_stripes {} {
  variable stripes
  variable stripe_locs
  
  foreach stripe_set [array names stripes] {
    # debug "$stripe_set [llength $stripes($stripe_set)]"
    if {[llength $stripes($stripe_set)] > 0} {
      set merged_stripes [shapes_to_polygonSet $stripes($stripe_set)]
      if {[array names stripe_locs $stripe_set] != ""} { 
        # debug "$stripe_locs($stripe_set)"
        set stripe_locs($stripe_set) [odb::orSet $stripe_locs($stripe_set) $merged_stripes]
      } else {
        set stripe_locs($stripe_set) $merged_stripes
      }
    }
    set stripes($stripe_set) {}
  }
}

# proc to generate follow pin layers or standard cell rails
proc generate_lower_metal_followpin_rails {} {
  variable block
  variable grid_data
  
  foreach row [$block getRows] {
    set orient [$row getOrient]
    set box [$row getBBox]
    switch -exact $orient {
      R0 {
        set vdd_y [$box yMax]
        set vss_y [$box yMin]
      }
      MX {
        set vdd_y [$box yMin]
        set vss_y [$box yMax]
      }
      default {
        critical 25 "Unexpected row orientation $orient for row [$row getName]"
      }
    }
    foreach lay [get_rails_layers] {
      set width [dict get $grid_data rails $lay width]
      set vdd_box [::odb::newSetFromRect [$box xMin] [expr $vdd_y - $width / 2] [$box xMax] [expr $vdd_y + $width / 2]]
      set vss_box [::odb::newSetFromRect [$box xMin] [expr $vss_y - $width / 2] [$box xMax] [expr $vss_y + $width / 2]]
      # debug "[$box xMin] [expr $vdd_y - $width / 2] [$box xMax] [expr $vdd_y + $width / 2]"
      add_stripe $lay "POWER" $vdd_box
      add_stripe $lay "GROUND" $vss_box
    }
  }
}


# proc for creating pdn mesh for upper metal layers
proc generate_upper_metal_mesh_stripes {tag layer layer_info area} {
  variable stripes_start_with

# If the grid_data defines a spacing for the layer, then:
#    place the second stripe spacing + width away from the first, 
# otherwise:
#    place the second stripe pitch / 2 away from the first, 
#
  set width [dict get $layer_info width]

  if {[get_dir $layer] == "hor"} {
    set offset [expr [lindex $area 1] + [dict get $layer_info offset]]
    if {$tag != $stripes_start_with} { ;#If not starting from bottom with this net, 
      if {[dict exists $layer_info spacing]} {
        set offset [expr {$offset + [dict get $layer_info spacing] + [dict get $layer_info width]}]
      } else {
        set offset [expr {$offset + ([dict get $layer_info pitch] / 2)}]
      }
    }
    for {set y $offset} {$y < [expr {[lindex $area 3] - [dict get $layer_info width]}]} {set y [expr {[dict get $layer_info pitch] + $y}]} {
      set box [::odb::newSetFromRect [lindex $area 0] [expr $y - $width / 2] [lindex $area 2] [expr $y + $width / 2]]
      add_stripe $layer $tag $box
    }
  } elseif {[get_dir $layer] == "ver"} {
    set offset [expr [lindex $area 0] + [dict get $layer_info offset]]

    if {$tag != $stripes_start_with} { ;#If not starting from bottom with this net, 
      if {[dict exists $layer_info spacing]} {
        set offset [expr {$offset + [dict get $layer_info spacing] + [dict get $layer_info width]}]
      } else {
        set offset [expr {$offset + ([dict get $layer_info pitch] / 2)}]
      }
    }
    for {set x $offset} {$x < [expr {[lindex $area 2] - [dict get $layer_info width]}]} {set x [expr {[dict get $layer_info pitch] + $x}]} {
      set box [::odb::newSetFromRect [expr $x - $width / 2] [lindex $area 1] [expr $x + $width / 2] [lindex $area 3]]
      add_stripe $layer $tag $box
    }
  } else {
    critical 26 "Invalid direction \"[get_dir $layer]\" for metal layer ${layer}. Should be either \"hor\" or \"ver\"."
  }
}

proc adjust_area_for_core_rings {layer area} {
  variable grid_data

  set core_offset [dict get $grid_data core_ring $layer core_offset]
  set width [dict get $grid_data core_ring $layer width]
  set spacing [dict get $grid_data core_ring $layer spacing]

  if {[get_dir $layer] == "hor"} {
    set extended_area [list \
      [expr [lindex $area 0] - $core_offset - $width - $spacing - $width / 2] \
      [lindex $area 1] \
      [expr [lindex $area 2] + $core_offset + $width + $spacing + $width / 2] \
      [lindex $area 3] \
    ]
  } else {
    set extended_area [list \
      [lindex $area 0] \
      [expr [lindex $area 1] - $core_offset - $width - $spacing - $width / 2] \
      [lindex $area 2] \
      [expr [lindex $area 3] + $core_offset + $width + $spacing + $width / 2] \
    ]
  }
  return $extended_area
}

## this is a top-level proc to generate PDN stripes and insert vias between these stripes
proc generate_stripes {tag} {
  variable plan_template
  variable template
  variable grid_data

  if {![dict exists $grid_data straps]} {return}
  foreach lay [dict keys [dict get $grid_data straps]] {
    # debug generate_stripes "    Layer $lay ..."

    #Upper layer stripes
    if {[dict exists $grid_data straps $lay width]} {
      set area [dict get $grid_data area]
      if {[dict exists $grid_data core_ring] && [dict exists $grid_data core_ring $lay]} {
        set area [adjust_area_for_core_rings $lay $area]
      }
      # debug "area=$area (spec area=[dict get $grid_data area])"
      generate_upper_metal_mesh_stripes $tag $lay [dict get $grid_data straps $lay] $area
    } else {
      foreach x [lsort -integer [dict keys $plan_template]] {
        foreach y [lsort -integer [dict keys [dict get $plan_template $x]]] {
          set template_name [dict get $plan_template $x $y]
          set layer_info [dict get $grid_data straps $lay $template_name]
          set area [list $x $y [expr $x + [dict get $template width]] [expr $y + [dict get $template height]]]
          # debug "Adding straps for $area"
          generate_upper_metal_mesh_stripes $tag $lay $layer_info $area
        }
      }
    }
  }
}

proc cut_blocked_areas {tag} {
  variable stripe_locs
  variable grid_data
  
  if {![dict exists  $grid_data straps]} {return}

  foreach layer_name [dict keys [dict get $grid_data straps]] {
    if {[dict exists $grid_data straps $layer_name width]} {
      set width [dict get $grid_data straps $layer_name width]
    } else {
      set template_name [lindex [dict get $grid_data template names] 0]
      set width [dict get $grid_data straps $layer_name $template_name width]
    }

    set blockages [get_blockages]
    if {[dict exists $blockages $layer_name]} {
      set stripe_locs($layer_name,$tag) [::odb::subtractSet $stripe_locs($layer_name,$tag) [dict get $blockages $layer_name]]

      # Trim any shapes that are less than the width of the wire
      set size_by [expr $width / 2 - 1]
      set trimmed_set [::odb::shrinkSet $stripe_locs($layer_name,$tag) $size_by]
      set stripe_locs($layer_name,$tag) [::odb::bloatSet $trimmed_set $size_by]
    }
  }
}

proc generate_grid_vias {tag net_name} {
  variable vias
  variable grid_data

  #Via stacks
  if {[dict exists $grid_data connect]} {
    # debug "Adding vias for $net_name ([llength [dict get $grid_data connect]] connections)..."
    foreach connection [dict get $grid_data connect] {
        set l1 [lindex $connection 0]
        set l2 [lindex $connection 1]
        # debug "    $l1 to $l2"
        set constraints {}
        if {[dict exists $connection constraints]} {
          set constraints [dict get $connection constraints]
        }
        # debug "    Constraints: $constraints"
        set connections [generate_via_stacks $l1 $l2 $tag $constraints]
        lappend vias [list net_name $net_name connections $connections]
    }
  }
  # debug "End"
}

proc get_core_ring_centre {type side layer_info} {
  variable grid_data

  set area [find_core_area]
  set xMin [lindex $area 0]
  set yMin [lindex $area 1]
  set xMax [lindex $area 2]
  set yMax [lindex $area 3]
  set core_offset [dict get $layer_info core_offset]
  set spacing [dict get $layer_info spacing]
  set width [dict get $layer_info width]

  # debug "area        $area"
  # debug "core_offset $core_offset"
  # debug "spacing     $spacing"
  # debug "width       $width"
  switch $type {
    "POWER" {
      switch $side {
        "t" {return [expr $yMax + $core_offset]}
        "b" {return [expr $yMin - $core_offset]} 
        "l" {return [expr $xMin - $core_offset]} 
        "r" {return [expr $xMax + $core_offset]}
      }
    }
    "GROUND" {
      switch $side {
        "t" {return [expr $yMax + $core_offset + $spacing + $width]}
        "b" {return [expr $yMin - $core_offset - $spacing - $width]} 
        "l" {return [expr $xMin - $core_offset - $spacing - $width]} 
        "r" {return [expr $xMax + $core_offset + $spacing + $width]}
      }
    }
  }
}

proc generate_core_rings {} {
  variable grid_data
  
  dict for {layer layer_info} [dict get $grid_data core_ring] {
    set area [find_core_area]
    set xMin [lindex $area 0]
    set yMin [lindex $area 1]
    set xMax [lindex $area 2]
    set yMax [lindex $area 3]
    set core_offset [dict get $layer_info core_offset]
    set spacing [dict get $layer_info spacing]
    set width [dict get $layer_info width]
    
    set inner_lx [expr $xMin - $core_offset]
    set inner_ly [expr $yMin - $core_offset]
    set inner_ux [expr $xMax + $core_offset]
    set inner_uy [expr $yMax + $core_offset]

    set outer_lx [expr $xMin - $core_offset - $spacing - $width]
    set outer_ly [expr $yMin - $core_offset - $spacing - $width]
    set outer_ux [expr $xMax + $core_offset + $spacing + $width]
    set outer_uy [expr $yMax + $core_offset + $spacing + $width]

    if {[get_dir $layer] == "hor"} {
      add_stripe $layer POWER \
        [odb::newSetFromRect \
          [expr $inner_lx - $width / 2] \
          [expr $inner_ly - $width / 2] \
          [expr $inner_ux + $width / 2] \
          [expr $inner_ly + $width / 2] \
        ]
      add_stripe $layer POWER \
        [odb::newSetFromRect \
          [expr $inner_lx - $width / 2] \
          [expr $inner_uy - $width / 2] \
          [expr $inner_ux + $width / 2] \
          [expr $inner_uy + $width / 2] \
        ]

      add_stripe $layer GROUND \
        [odb::newSetFromRect \
          [expr $outer_lx - $width / 2] \
          [expr $outer_ly - $width / 2] \
          [expr $outer_ux + $width / 2] \
          [expr $outer_ly + $width / 2] \
        ]

      add_stripe $layer GROUND \
        [odb::newSetFromRect \
          [expr $outer_lx - $width / 2] \
          [expr $outer_uy - $width / 2] \
          [expr $outer_ux + $width / 2] \
          [expr $outer_uy + $width / 2] \
        ]
    } else {
      add_stripe $layer POWER \
        [odb::newSetFromRect \
          [expr $inner_lx - $width / 2] \
          [expr $inner_ly - $width / 2] \
          [expr $inner_lx + $width / 2] \
          [expr $inner_uy + $width / 2] \
        ]
      add_stripe $layer POWER \
        [odb::newSetFromRect \
          [expr $inner_ux - $width / 2] \
          [expr $inner_ly - $width / 2] \
          [expr $inner_ux + $width / 2] \
          [expr $inner_uy + $width / 2] \
        ]

      add_stripe $layer GROUND \
        [odb::newSetFromRect \
          [expr $outer_lx - $width / 2] \
          [expr $outer_ly - $width / 2] \
          [expr $outer_lx + $width / 2] \
          [expr $outer_uy + $width / 2] \
        ]

      add_stripe $layer GROUND \
        [odb::newSetFromRect \
          [expr $outer_ux - $width / 2] \
          [expr $outer_ly - $width / 2] \
          [expr $outer_ux + $width / 2] \
          [expr $outer_uy + $width / 2] \
        ]
    }
  }
}

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

proc import_macro_boundaries {} {
  variable libs
  variable macros
  variable instances

  # debug "start"
  set macros {}
  foreach lib $libs {
    foreach cell [$lib getMasters] {
      if {[$cell getType] != "BLOCK"} {continue}

      dict set macros [$cell getName] [list \
        width  [$cell getWidth] \
        height [$cell getHeight] \
      ]
    }
  }

  set instances [import_def_components [dict keys $macros]]

  set boundary [odb::newSetFromRect {*}[get_core_area]]
    
  foreach instance [dict keys $instances] {
    set macro_name [dict get $instances $instance macro]

    set llx [dict get $instances $instance xmin]
    set lly [dict get $instances $instance ymin]
    set urx [dict get $instances $instance xmax]
    set ury [dict get $instances $instance ymax]

    # If there are no shapes left after 'and'ing the boundard with the cell, then
    # the cell lies outside the area where we are adding a power grid.
    set box [odb::newSetFromRect $llx $lly $urx $ury]
    if {[llength [odb::getPolygons [odb::andSet $boundary $box]]] == 0} {
      # debug "Instance $instance does not lie in the cell area"
      set instances [dict remove $instances $instance]
      continue
    }

    dict set instances $instance macro_boundary [list $llx $lly $urx $ury]

    set halo [dict get $instances $instance halo]
    set llx [expr round($llx - [lindex $halo 0])]
    set lly [expr round($lly - [lindex $halo 1])]
    set urx [expr round($urx + [lindex $halo 2])]
    set ury [expr round($ury + [lindex $halo 3])]

    dict set instances $instance halo_boundary [list $llx $lly $urx $ury]
  }
  
  # debug "end"
  
}

proc import_def_components {macros} {
  variable design_data
  variable block
  set instances {}
  # debug "$macros"

  foreach inst [$block getInsts] {
    set macro_name [[$inst getMaster] getName]
    if {[lsearch -exact $macros $macro_name] != -1} {
      # debug "macro $macro_name"
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
      # debug "data $data"

      dict set instances [$inst getName] $data
    }
  }
  # debug "end"

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

proc export_opendb_vias {} {
  variable physical_viarules
  variable block
  variable tech
  # debug "[llength $physical_viarules]"
  dict for {name rules} $physical_viarules {
    foreach rule $rules {
      # debug "$rule"
      set via [$block findVia [dict get $rule name]]
      if {$via == "NULL"} {
        set via [odb::dbVia_create $block [dict get $rule name]]
        # debug "Via $via"

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
  }
  # debug "end"
}

proc export_opendb_specialnet {net_name signal_type} {
  variable block
  variable instances
  variable metal_layers
  variable tech 
  variable stripe_locs
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
      if {[$mterm getSigType] == $signal_type && [dict exists $global_connections $net_name]} {
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

  # debug "layers - $metal_layers"
  foreach lay $metal_layers {
    if {[array names stripe_locs "$lay,$signal_type"] == ""} {continue}

    set layer [find_layer $lay]
    foreach shape [::odb::getPolygons $stripe_locs($lay,$signal_type)] {
      set points [::odb::getPoints $shape]
      if {[llength $points] != 4} {
        variable def_units
        warning 6 "Unexpected number of points in shape ($lay $signal_type [llength $points])"
        set str "    "
        foreach point $points {set str "$str ([expr 1.0 * [$point getX] / $def_units ] [expr 1.0 * [$point getY] / $def_units]) "}
        warning 7 $str
        continue
      }
      set xMin [expr min([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
      set xMax [expr max([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
      set yMin [expr min([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]
      set yMax [expr max([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]

      set width [expr $xMax - $xMin]
      set height [expr $yMax - $yMin]

      set wire_type "STRIPE"
      if {[is_rails_layer $lay]} {set wire_type "FOLLOWPIN"}
      # debug "$xMin $yMin $xMax $yMax $wire_type"
      odb::dbSBox_create $swire $layer $xMin $yMin $xMax $yMax $wire_type
    }
  }

  variable vias
  # debug "vias - [llength $vias]"
  foreach via $vias {
    if {[dict get $via net_name] == $net_name} {
      # For each layer between l1 and l2, add vias at the intersection
      foreach via_inst [dict get $via connections] {
        # debug "$via_inst"
        set via_name [dict get $via_inst name]
        set x        [dict get $via_inst x]
        set y        [dict get $via_inst y]
        # debug "$via_name $x $y [$block findVia $via_name]"
        odb::dbSBox_create $swire [$block findVia $via_name] $x $y "STRIPE"
        # debug "via created"
      }
    }
  }
  # debug "end"
}
  
proc export_opendb_specialnets {} {
  variable block
  variable design_data
  
  foreach net_name [dict get $design_data power_nets] {
    export_opendb_specialnet $net_name "POWER"
  }

  foreach net_name [dict get $design_data ground_nets] {
    export_opendb_specialnet $net_name "GROUND"
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
  if {(int($start - $llx) % $site_width) == 0} {
      set x $start
  } else {
      set offset [expr { int($start - $llx) % $site_width}]
      set x [expr {$start + $site_width - $offset}]
  }

  set num  [expr {($end - $x)/$site_width}]
  
  odb::dbRow_create $block ROW_$row_index $site $x $height [orientation $height] "HORIZONTAL" $num $site_width
  incr row_index
}

## procedure for file existence check, returns 0 if file does not exist or file exists, but empty
proc file_exists_non_empty {filename} {
  return [expr [file exists $filename] && [file size $filename] > 0]
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
    default {critical 27 "Illegal orientation $orientation specified"}
  }
  return [list \
    [expr [lindex $new_box 0] + [lindex $origin 0]] \
    [expr [lindex $new_box 1] + [lindex $origin 1]] \
    [expr [lindex $new_box 2] + [lindex $origin 0]] \
    [expr [lindex $new_box 3] + [lindex $origin 1]] \
  ]
}

proc set_template_size {width height} {
  variable template
  variable def_units
  
  dict set template width [expr round($width * $def_units)]
  dict set template height [expr round($height * $def_units)]
}

proc get_memory_instance_pg_pins {} {
  variable block
  variable metal_layers

  # debug "start"
  set boundary [odb::newSetFromRect {*}[get_core_area]]

  foreach inst [$block getInsts] {
    set inst_name [$inst getName]
    set master [$inst getMaster]

    if {[$master getType] != "BLOCK"} {continue}

    # If there are no shapes left after 'and'ing the boundard with the cell, then
    # the cell lies outside the area where we are adding a power grid.
    set bbox [$inst getBBox]
    set box [odb::newSetFromRect [$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]]
    if {[llength [odb::getPolygons [odb::andSet $boundary $box]]] == 0} {
      # debug "Instance [$inst getName] does not lie in the cell area"
      continue
    }
    
    # debug "cell name - [$master getName]"

    foreach term_name [concat [get_macro_power_pins $inst_name] [get_macro_ground_pins $inst_name]] {
      set inst_term [$inst findITerm $term_name]
      if {$inst_term == "NULL"} {continue}
      
      set mterm [$inst_term getMTerm]
      set type [$mterm getSigType]
      set pin_shapes {}
      foreach mPin [$mterm getMPins] {
        foreach geom [$mPin getGeometry] {
          set layer [[$geom getTechLayer] getName]
          if {[lsearch -exact $metal_layers $layer] == -1} {continue}

          set box [transform_box [$geom xMin] [$geom yMin] [$geom xMax] [$geom yMax] [$inst getOrigin] [$inst getOrient]]

          set width  [expr abs([lindex $box 2] - [lindex $box 0])]
          set height [expr abs([lindex $box 3] - [lindex $box 1])]

          if {$width > $height} {
            set layer_name ${layer}_PIN_hor
          } else {
            set layer_name ${layer}_PIN_ver
          }
          set pin_shape [odb::newSetFromRect {*}$box]
          # debug "$pin_shapes"
          if {![dict exists $pin_shapes $layer_name]} {
            dict set pin_shapes $layer_name $pin_shape
          } else {
            dict set pin_shapes $layer_name [odb::orSet [dict get $pin_shapes $layer_name] $pin_shape]
          }            
        }
      }
      dict for {layer_name shapes} $pin_shapes {
        add_stripe $layer_name $type $shapes
      }
    }    
  }
  # debug "Total walltime till macro pin geometry creation = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"
  # debug "end"
}

proc set_core_area {xmin ymin xmax ymax} {
  variable design_data

  dict set design_data config core_area [list $xmin $ymin $xmax $ymax]
}

proc get_core_area {} {
  variable design_data

  return [get_extent [get_stdcell_area]]
}

proc write_pdn_strategy {} {
  variable design_data
  
  if {[dict exists $design_data grid]} {
    set_pdn_string_property_value "strategy" [dict get $design_data grid]
  }
}

proc init {{PDN_cfg "PDN.cfg"}} {
  variable db
  variable block
  variable tech
  variable libs
  variable design_data
  variable def_output
  variable default_grid_data
  variable design_name
  variable stripe_locs
  variable site
  variable row_height
  variable site_width
  variable site_name
  variable metal_layers
  variable def_units
  variable stripes_start_with
  variable rails_start_with
  variable physical_viarules
  variable stdcell_area
  
#    set ::start_time [clock clicks -milliseconds]
  if {![file_exists_non_empty $PDN_cfg]} {
    critical 28 "File $PDN_cfg does not exist, or exists but empty"
  }

  set tech [$db getTech]
  set libs [$db getLibs]
  set block [[$db getChip] getBlock]
  set def_units [$block getDefUnits]
  set design_name [$block getName]

  set design_data {}
  set physical_viarules {}
  set stdcell_area ""
 
  # debug "start" 
  source $PDN_cfg
  write_pdn_strategy 
  
  init_metal_layers
  init_via_tech
 
  set die_area [$block getDieArea]
  information 8 "Design Name is $design_name"
  set def_output "${design_name}_pdn.def"
  
  # debug "examine vars"
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
  information 9 "Reading technology data"

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
      critical 29 "Illegal number of elements defined for ::halo \"$::halo\" (1, 2 or 4 allowed)"
    }
  } else {
    set default_halo "0 0 0 0"
  }

  dict set design_data config def_output   $def_output
  dict set design_data config design       $design_name
  dict set design_data config die_area     [list [$die_area xMin]  [$die_area yMin] [$die_area xMax] [$die_area yMax]]
  dict set design_data config default_halo [lmap x $default_halo {expr $x * $def_units}]
         
  array unset stripe_locs

  ########################################
  # Remove existing power/ground nets
  #######################################
  foreach pg_net [concat [dict get $design_data power_nets] [dict get $design_data ground_nets]] {
    set net [$block findNet $pg_net]
    if {$net != "NULL"} {
      odb::dbNet_destroy $net
    }
  }

  if {[dict exists $design_data grid stdcell]} {
    set default_grid_data [dict get $design_data grid stdcell [lindex [dict keys [dict get $design_data grid stdcell]] 0]]
  }
  
  # debug "Set the core area"
  # Set the core area
  if {[info vars ::core_area_llx] != "" && [info vars ::core_area_lly] != "" && [info vars ::core_area_urx] != "" && [info vars ::core_area_ury] != ""} {
     # The core area is larger than the stdcell area by half a rail, since the stdcell rails extend beyond the rails

     set_core_area \
       [expr round($::core_area_llx * $def_units)] \
       [expr round($::core_area_lly * $def_units)] \
       [expr round($::core_area_urx * $def_units)] \
       [expr round($::core_area_ury * $def_units)]
  } else {
    set_core_area {*}[get_extent [get_stdcell_plus_area]]
  }
  
  ########################################
  # Creating blockages based on macro locations
  #######################################
  # debug "import_macro_boundaries"
  import_macro_boundaries

  # debug "get_memory_instance_pg_pins"
  get_memory_instance_pg_pins

  ##### Basic sanity checks to see if inputs are given correctly
  foreach layer [get_rails_layers] {
    if {[lsearch -exact $metal_layers $layer] < 0} {
      critical 30 "Layer specified for std. cell rails '$layer' not in list of layers."
    }
  }
  # debug "end"

  return $design_data
}

proc convert_layer_spec_to_def_units {data} {
  variable def_units
  
  foreach key {width pitch spacing offset core_offset} {
    if {[dict exists $data $key]} {
      dict set data $key [expr round([dict get $data $key] * $def_units)]
    }
  }
  return $data
}

proc specify_grid {type specification} {
  variable design_data
  
  set spec $specification
  if {![dict exists $spec name]} {
    if {[dict exists $design_data grid $type]} {
      set index [expr [dict size [dict get $design_data grid $type]] + 1]
    } else {
      set index 1
    }
    dict set spec name "${type}_$index"
  }
  set spec_name [dict get $spec name]
  
  if {[dict exists $specification core_ring]} {
    dict for {layer data} [dict get $specification core_ring] {
      dict set spec core_ring $layer [convert_layer_spec_to_def_units $data]
    }
  }
  
  if {[dict exists $specification rails]} {
    dict for {layer data} [dict get $specification rails] {
      dict set spec rails $layer [convert_layer_spec_to_def_units $data]
      if {[dict exists $specification template]} {
        foreach template [dict get $specification template names] {
          if {[dict exists $specification layers $layer $template]} {
            dict set spec rails $layer $template [convert_layer_spec_to_def_units [dict get $specification rails $layer $template]]
          }
        }
      }
    }
  }
  
  if {[dict exists $specification straps]} {
    dict for {layer data} [dict get $specification straps] {
      dict set spec straps $layer [convert_layer_spec_to_def_units $data]
      if {[dict exists $specification template]} {
        foreach template [dict get $specification template names] {
          if {[dict exists $specification straps $layer $template]} {
            dict set spec straps $layer $template [convert_layer_spec_to_def_units [dict get $specification straps $layer $template]]
          }
        }
      }
    }
  }
  
  if {[dict exists $specification template]} {
    set_template_size {*}[dict get $specification template size]
  }
  
  dict set design_data grid $type $spec_name $spec
}

proc get_quadrant {x y} {
  variable design_data
  
  set die_area [dict get $design_data config die_area]
  set dw [expr [lindex $die_area 2] - [lindex $die_area 0]]
  set dh [expr [lindex $die_area 3] - [lindex $die_area 1]]
  
  set test_x [expr $x - [lindex $die_area 0]]
  set test_y [expr $y - [lindex $die_area 1]]
  # debug "$dw * $test_y ([expr $dw * $test_y]) > expr $dh * $test_x ([expr $dh * $test_x])"
  if {$dw * $test_y > $dh * $test_x} {
    # Top or left
    if {($dw * $test_y) + ($dh * $test_x) > ($dw * $dh)} {
      # Top or right
      return "t"
    } else {
      # Bottom or left
      return "l"
    }
  } else {
    # Bottom or right
    if {($dw * $test_y) + ($dh * $test_x) > ($dw * $dh)} {
      # Top or right
      return "r"
    } else {
      # Bottom or left
      return "b"
    }
  }
}

proc get_core_facing_pins {instance pin_name side layer} {
  variable block
  set geoms {}
  set core_pins {}
  set inst [$block findInst [dict get $instance name]]
  set pins [[[$inst findITerm $pin_name] getMTerm] getMPins]
  
  # debug "start"
  foreach pin $pins {
    foreach geom [$pin getGeometry] {
      if {[[$geom getTechLayer] getName] != $layer} {continue}
      lappend geoms $geom
    }
  }
  # debug "$pins"
  foreach geom $geoms {
    set ipin [transform_box [$geom xMin] [$geom yMin] [$geom xMax] [$geom yMax] [$inst getOrigin] [$inst getOrient]]
    # debug "$ipin [[$inst getBBox] xMin] [[$inst getBBox] yMin] [[$inst getBBox] xMax] [[$inst getBBox] yMax] "
    switch $side {
      "t" {
        if {[lindex $ipin 1] == [[$inst getBBox] yMin]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 2] + [lindex $ipin 0]) / 2] \
            width [expr [lindex $ipin 2] - [lindex $ipin 0]] \
          ]
        }
      }
      "b" {
        if {[lindex $ipin 3] == [[$inst getBBox] yMax]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 2] + [lindex $ipin 0]) / 2] \
            width [expr [lindex $ipin 2] - [lindex $ipin 0]] \
          ]
        }
      }
      "l" {
        if {[lindex $ipin 2] == [[$inst getBBox] xMax]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 3] + [lindex $ipin 1]) / 2] \
            width [expr [lindex $ipin 3] - [lindex $ipin 1]] \
          ]
        }
      } 
      "r" {
        if {[lindex $ipin 0] == [[$inst getBBox] xMin]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 3] + [lindex $ipin 1]) / 2] \
            width [expr [lindex $ipin 3] - [lindex $ipin 1]] \
          ]
        }
      }
    }
  }
  # debug "$core_pins"
  return $core_pins
}

proc connect_pads_to_core_ring {type pin_name pads} {
  variable grid_data
  # debug "start - pads $pads"
  dict for {inst_name instance} [import_def_components $pads] {
    # debug "inst $inst_name"
    set side [get_quadrant [dict get $instance x] [dict get $instance y]]
    # debug "inst [dict get $instance name] x [dict get $instance x] y [dict get $instance y] side $side"
    switch $side {
      "t" {
        set required_direction "ver"
      }
      "b" {
        set required_direction "ver"
      }
      "l" {
        set required_direction "hor"
      }
      "r" {
        set required_direction "hor"
      }
    }
    foreach non_pref_layer [dict keys [dict get $grid_data core_ring]] {
      if {[get_dir $non_pref_layer] != $required_direction} {
        set non_pref_layer_info [dict get $grid_data core_ring $non_pref_layer]
        break
      }
    }
    # debug "find_layer"
    foreach pref_layer [dict keys [dict get $grid_data core_ring]] {
      if {[get_dir $pref_layer] == $required_direction} {
        break
      }
    }
    switch $side {
      "t" {
        set y_min [expr [get_core_ring_centre $type $side $non_pref_layer_info] - [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set y_min_blk [expr $y_min - [dict get $grid_data core_ring $non_pref_layer spacing]]
        set y_max [dict get $instance ymin]
        # debug "t: [dict get $instance xmin] $y_min_blk [dict get $instance xmax] [dict get $instance ymax]"
        add_blockage $pref_layer [odb::newSetFromRect [dict get $instance xmin] $y_min_blk [dict get $instance xmax] [dict get $instance ymax]]
      }
      "b" {
        # debug "[get_core_ring_centre $type $side $non_pref_layer_info] + [dict get $grid_data core_ring $non_pref_layer width] / 2"
        set y_max [expr [get_core_ring_centre $type $side $non_pref_layer_info] + [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set y_max_blk [expr $y_max + [dict get $grid_data core_ring $non_pref_layer spacing]]
        set y_min [dict get $instance ymax]
        # debug "b: [dict get $instance xmin] [dict get $instance ymin] [dict get $instance xmax] $y_max"
        add_blockage $pref_layer [odb::newSetFromRect [dict get $instance xmin] [dict get $instance ymin] [dict get $instance xmax] $y_max_blk]
        # debug "end b"
      }
      "l" {
        set x_max [expr [get_core_ring_centre $type $side $non_pref_layer_info] + [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set x_max_blk [expr $x_max + [dict get $grid_data core_ring $non_pref_layer spacing]]
        set x_min [dict get $instance xmax]
        # debug "l: [dict get $instance xmin] [dict get $instance ymin] $x_max [dict get $instance ymax]"
        add_blockage $pref_layer [odb::newSetFromRect [dict get $instance xmin] [dict get $instance ymin] $x_max_blk [dict get $instance ymax]]
      }
      "r" {
        set x_min [expr [get_core_ring_centre $type $side $non_pref_layer_info] - [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set x_min_blk [expr $x_min - [dict get $grid_data core_ring $non_pref_layer spacing]]
        set x_max [dict get $instance xmin]
        # debug "r: $x_min_blk [dict get $instance ymin] [dict get $instance xmax] [dict get $instance ymax]"
        add_blockage $pref_layer [odb::newSetFromRect $x_min_blk [dict get $instance ymin] [dict get $instance xmax] [dict get $instance ymax]]
      }
    }

    # debug "$pref_layer"
    foreach pin_geometry [get_core_facing_pins $instance $pin_name $side $pref_layer] {
      set centre [dict get $pin_geometry centre]
      set width  [dict get $pin_geometry width]
      if {$required_direction == "hor"} {
        # debug "added_strap $pref_layer $type $x_min [expr $centre - $width / 2] $x_max [expr $centre + $width / 2]"
        add_stripe $pref_layer "PAD_$type" [odb::newSetFromRect $x_min [expr $centre - $width / 2] $x_max [expr $centre + $width / 2]]
      } else {
        # debug "added_strap $pref_layer $type [expr $centre - $width / 2] $y_min [expr $centre + $width / 2] $y_max"
        add_stripe $pref_layer "PAD_$type" [odb::newSetFromRect [expr $centre - $width / 2] $y_min [expr $centre + $width / 2] $y_max]
      }
    }
  }
  # debug "end"
}

proc add_pad_straps {tag} {
  variable stripe_locs
  
  foreach pad_connection [array names stripe_locs "*,PAD_*"] {
    if {![regexp "(.*),PAD_$tag" $pad_connection - layer]} {continue}
    # debug "$pad_connection"
    if {[array names stripe_locs "$layer,$tag"] != ""} {
      # debug add_pad_straps "Before: $layer [llength [::odb::getPolygons $stripe_locs($layer,$tag)]]"
      # debug add_pad_straps "Adding: [llength [::odb::getPolygons $stripe_locs($pad_connection)]]"
      add_stripe $layer $tag $stripe_locs($pad_connection)
      # debug add_pad_straps "After:  $layer [llength [::odb::getPolygons $stripe_locs($layer,$tag)]]"
    }
  }
}

proc print_spacing_table {layer_name} {
  set layer [find_layer $layer_name]
  if {[$layer hasTwoWidthsSpacingRules]} {
    set table_size [$layer getTwoWidthsSpacingTableNumWidths]
    for {set i 0} {$i < $table_size} {incr i} {
      set width [$layer getTwoWidthsSpacingTableWidth $i]
      puts -nonewline "WIDTH $width "
      if {[$layer getTwoWidthsSpacingTableHasPRL $i]} {
        set prl [$layer getTwoWidthsSpacingTablePRL $i] 
        puts -nonewline "PRL $prl "
      }
      puts -nonewline "[$layer getTwoWidthsSpacingTableEntry 0 $i] "
    }
    puts ""
  }
}

proc get_twowidths_table {table_type} {
  variable metal_layers
  set twowidths_table {}
  
  foreach layer_name $metal_layers {
    set spacing_table [get_spacingtables $layer_name]
    set prls {}

    if {[dict exists $spacing_table TWOWIDTHS $table_type]} {
      set layer_spacing_table [dict get $spacing_table TWOWIDTHS $table_type]
      set table_size [dict size $layer_spacing_table]
      set table_widths [dict keys $layer_spacing_table]

      for {set i 0} {$i < $table_size} {incr i} {

        set width [lindex $table_widths $i]
        set spacing [lindex [dict get $layer_spacing_table $width spacings] $i]

        if {[dict get $layer_spacing_table $width prl] != 0} {
          set prl [dict get $layer_spacing_table $width prl] 
          set update_prls {}
          dict for {prl_entry prl_setting} $prls {
            if {$prl <= [lindex $prl_entry 0]} {break}
            dict set update_prls $prl_entry $prl_setting
            dict set twowidths_table $layer_name $width $prl_entry $prl_setting
          }
          dict set update_prls $prl $spacing
          dict set twowidths_table $layer_name $width $prl $spacing
          set prls $update_prls
        } else {
          set prls {}
          dict set prls 0 $spacing
          dict set twowidths_table $layer_name $width 0 $spacing
        }
      }
    }
  }
  
  return $twowidths_table
}

proc get_twowidths_tables {} {
  variable twowidths_table
  variable twowidths_table_wrongdirection
  
  set twowidths_table [get_twowidths_table NONE]
  set twowidths_table_wrongdirection [get_twowidths_table WRONGDIRECTION]
}

proc select_from_table {table width} {
  foreach value [lreverse [lsort -integer [dict keys $table]]] {
    if {$width > $value} {
      return $value
    }
  }
  return [lindex [dict keys $table] 0]
}

proc get_preferred_direction_spacing {layer_name width prl} {
  variable twowidths_table

  # debug "$layer_name $width $prl"
  # debug "twowidths_table $twowidths_table"
  if {$twowidths_table == {}} {
    return [[find_layer $layer_name] getSpacing]
  } else {
    set width_key [select_from_table [dict get $twowidths_table $layer_name] $width]
    set prl_key   [select_from_table [dict get $twowidths_table $layer_name $width_key] $prl]
  }
  
  return [dict get $twowidths_table $layer_name $width_key $prl_key]
}

proc get_nonpreferred_direction_spacing {layer_name width prl} {
  variable twowidths_table_wrongdirection
  
  # debug "twowidths_table_wrong_direction $twowidths_table_wrongdirection"
  if {[dict exists $twowidths_table_wrongdirection $layer_name]} {
    set width_key [select_from_table [dict get $twowidths_table_wrongdirection $layer_name] $width]
    set prl_key   [select_from_table [dict get $twowidths_table_wrongdirection $layer_name $width_key] $prl]
  } else {
    return [get_preferred_direction_spacing $layer_name $width $prl]
  }
  
  return [dict get $twowidths_table_wrongdirection $layer_name $width_key $prl_key]
}

proc create_obstructions {layer_name polygons} {
  set layer [find_layer $layer_name]
  set min_spacing [get_preferred_direction_spacing $layer_name 0 0]

  # debug "Num polygons [llength $polygons]"

  foreach polygon $polygons {
    set points [::odb::getPoints $polygon]
    if {[llength $points] != 4} {
      warning 6 "Unexpected number of points in stripe of $layer_name"
      continue
    }
    set xMin [expr min([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set xMax [expr max([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set yMin [expr min([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]
    set yMax [expr max([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]

    if {[get_dir $layer_name] == "hor"} {
      set required_spacing_pref    [get_preferred_direction_spacing $layer_name [expr $yMax - $yMin] [expr $xMax - $xMin]]
      set required_spacing_nonpref [get_nonpreferred_direction_spacing $layer_name [expr $xMax - $xMin] [expr $yMax - $yMin]]

      set y_change [expr $required_spacing_pref    - $min_spacing]
      set x_change [expr $required_spacing_nonpref - $min_spacing]
    } else {
      set required_spacing_pref    [get_preferred_direction_spacing $layer_name [expr $xMax - $xMin] [expr $yMax - $yMin]]
      set required_spacing_nonpref [get_nonpreferred_direction_spacing $layer_name [expr $yMax - $yMin] [expr $xMax - $xMin]]

      set x_change [expr $required_spacing_pref    - $min_spacing]
      set y_change [expr $required_spacing_nonpref - $min_spacing]
    }

    create_obstruction_object_blockage $layer $min_spacing [expr $xMin - $x_change] [expr $yMin - $y_change] [expr $xMax + $x_change] [expr $yMax + $y_change]
  }
}

proc combine {lside rside} {
  # debug "l [llength $lside] r [llength $rside]"
  if {[llength $lside] > 1} {
    set lside [combine [lrange $lside 0 [expr [llength $lside] / 2 - 1]] [lrange $lside [expr [llength $lside] / 2] end]]
  }
  if {[llength $rside] > 1} {
    set rside [combine [lrange $rside 0 [expr [llength $rside] / 2 - 1]] [lrange $rside [expr [llength $rside] / 2] end]]
  }
  return [odb::orSet $lside $rside]
}

proc shapes_to_polygonSet {shapes} {
  if {[llength $shapes] == 1} {
    return $shapes
  }
  return [combine [lrange $shapes 0 [expr [llength $shapes] / 2 - 1]] [lrange $shapes [expr [llength $shapes] / 2] end]]
}

proc generate_obstructions {layer_name} {
  variable stripe_locs
  
  # debug "layer $layer_name"
  get_twowidths_tables

  set block_shapes {}
  foreach tag {"POWER" "GROUND"} {
    if {[array names stripe_locs $layer_name,$tag] == ""} {
      # debug "No polygons on $layer_name,$tag"
      continue
    }
    if {$block_shapes == {}} {
      set block_shapes $stripe_locs($layer_name,$tag)
    } else {
      set block_shapes [odb::orSet $block_shapes $stripe_locs($layer_name,$tag)]
    }
  }
  set via_shapes 0
  variable vias
  # debug "vias - [llength $vias]"
  foreach via $vias {
    # For each layer between l1 and l2, add vias at the intersection
    foreach via_inst [dict get $via connections] {
      # debug "$via_inst"
      set via_name [dict get $via_inst name]
      set x        [dict get $via_inst x]
      set y        [dict get $via_inst y]

      set lower_layer_name [lindex [dict get $via_inst layers] 0]
      set upper_layer_name [lindex [dict get $via_inst layers] 2]

      if {$lower_layer_name == $layer_name && [dict exists $via_inst lower_rect]} {        
        lappend block_shapes [odb::newSetFromRect {*}[transform_box {*}[dict get $via_inst lower_rect] [list $x $y] "R0"]]
        incr via_shapes
      } elseif {$upper_layer_name == $layer_name && [dict exists $via_inst upper_rect]} {
        lappend block_shapes [odb::newSetFromRect {*}[transform_box {*}[dict get $via_inst upper_rect] [list $x $y] "R0"]]
        incr via_shapes
      }
    }
  }
  # debug "Via shapes $layer_name $via_shapes"
  if {$block_shapes != {}} {
  # debug "create_obstructions [llength $block_shapes]"
    create_obstructions $layer_name [odb::getPolygons [shapes_to_polygonSet $block_shapes]]
  }
  # debug "end"
}

proc create_obstruction_object_blockage {layer min_spacing xMin yMin xMax yMax} {
  variable block
  

  set layer_pitch [get_pitch $layer]
  set layer_width [$layer getWidth]
  # debug "Layer - [$layer getName], pitch $layer_pitch, width $layer_width"
  set tracks [$block findTrackGrid $layer]
  set offsetX [lindex [$tracks getGridX] 0]
  set offsetY [lindex [$tracks getGridY] 0]

  # debug "OBS: [$layer getName] $xMin $yMin $xMax $yMax (dx [expr $xMax - $xMin] dy [expr $yMax - $yMin])"
  # debug "Offsets: x $offsetX y $offsetY"
  set relative_xMin [expr $xMin - $offsetX]
  set relative_xMax [expr $xMax - $offsetX]
  set relative_yMin [expr $yMin - $offsetY]
  set relative_yMax [expr $yMax - $offsetY]
  # debug "relative to core area $relative_xMin $relative_yMin $relative_xMax $relative_yMax"
  
  # debug "OBS: [$layer getName] $xMin $yMin $xMax $yMax"
  # Determine which tracks are blocked
  if {[get_dir [$layer getName]] == "hor"} {
    set pitch_start [expr $relative_yMin / $layer_pitch]
    if {$relative_yMin % $layer_pitch >= ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_yMax / $layer_pitch]
    if {$relative_yMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    # debug "pitch: start $pitch_start end $pitch_end"
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      set obs [odb::dbObstruction_create $block $layer \
        $xMin \
        [expr $i * $layer_pitch + $offsetY - $layer_width / 2] \
        $xMax \
        [expr $i * $layer_pitch + $offsetY + $layer_width / 2] \
      ]
    }
  } else {
    set pitch_start [expr $relative_xMin / $layer_pitch]
    if {$relative_xMin % $layer_pitch >= ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_xMax / $layer_pitch]
    if {$relative_xMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    # debug "pitch: start $pitch_start end $pitch_end"
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      set obs [odb::dbObstruction_create $block $layer \
        [expr $i * $layer_pitch + $offsetX - $layer_width / 2] \
        $yMin \
        [expr $i * $layer_pitch + $offsetX + $layer_width / 2] \
        $yMax \
      ]
    }
  }
}

proc create_obstruction_object_net {layer min_spacing xMin yMin xMax yMax} {
  variable block
  variable obstruction_index
  
  incr obstruction_index
  set net_name "obstruction_$obstruction_index"
  if {[set obs_net [$block findNet $net_name]] == "NULL"} {
    set obs_net [odb::dbNet_create $block $net_name]
  }
  # debug "obs_net [$obs_net getName]"
  if {[set wire [$obs_net getWire]] == "NULL"} {
    set wire [odb::dbWire_create $obs_net]
  }
  # debug "Wire - net [[$wire getNet] getName]"
  set encoder [odb::dbWireEncoder]
  $encoder begin $wire
  
  set layer_pitch [$layer getPitch]
  set layer_width [$layer getWidth]
  # debug "Layer - [$layer getName], pitch $layer_pitch, width $layer_width"
  set core_area [get_core_area]
  # debug "core_area $core_area"
  set relative_xMin [expr $xMin - [lindex $core_area 0]]
  set relative_xMax [expr $xMax - [lindex $core_area 0]]
  set relative_yMin [expr $yMin - [lindex $core_area 1]]
  set relative_yMax [expr $yMax - [lindex $core_area 1]]
  # debug "relative to core area $relative_xMin $relative_yMin $relative_xMax $relative_yMax"
  
  # debug "OBS: [$layer getName] $xMin $yMin $xMax $yMax"
  # Determine which tracks are blocked
  if {[get_dir [$layer getName]] == "hor"} {
    set pitch_start [expr $relative_yMin / $layer_pitch]
    if {$relative_yMin % $layer_pitch > ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_yMax / $layer_pitch]
    if {$relative_yMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      $encoder newPath $layer ROUTED
      $encoder addPoint [expr $relative_xMin + [lindex $core_area 0]] [expr $i * $layer_pitch + [lindex $core_area 1]]
      $encoder addPoint [expr $relative_xMax + [lindex $core_area 0]] [expr $i * $layer_pitch + [lindex $core_area 1]]
    }
  } else {
    set pitch_start [expr $relative_xMin / $layer_pitch]
    if {$relative_xMin % $layer_pitch > ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_xMax / $layer_pitch]
    if {$relative_xMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      $encoder newPath $layer ROUTED
      $encoder addPoint [expr $i * $layer_pitch + [lindex $core_area 0]] [expr $relative_yMin + [lindex $core_area 1]]
      $encoder addPoint [expr $i * $layer_pitch + [lindex $core_area 0]] [expr $relative_yMax + [lindex $core_area 1]]
    }
  }
  $encoder end
}

proc add_grid {} {
  variable design_data
  variable grid_data
  
  if {[dict exists $grid_data core_ring]} {
    generate_core_rings
    if {[dict exists $grid_data pwr_pads]} {
      connect_pads_to_core_ring \
        "GROUND" \
        [lindex [dict get $design_data ground_nets] 0] \
        [dict get $grid_data gnd_pads]
    }
    if {[dict exists $grid_data pwr_pads]} {
      connect_pads_to_core_ring \
        "POWER" \
        [lindex [dict get $design_data power_nets] 0] \
        [dict get $grid_data pwr_pads]
    }
  }
  
  # debug "Adding stdcell rails"
  # debug "area: [dict get $grid_data area]"
  if {[dict exists $grid_data rails]} {
    set area [dict get $grid_data area]
    generate_lower_metal_followpin_rails
  }

  ## Power nets
  # debug "Power straps"
  foreach pwr_net [dict get $design_data power_nets] {
    set tag "POWER"
    generate_stripes $tag
  }
  ## Ground nets
  # debug "Ground straps"
  foreach gnd_net [dict get $design_data ground_nets] {
    set tag "GROUND"
    generate_stripes $tag
  }
  merge_stripes

  ## Power nets
  # debug "Power straps"
  foreach pwr_net [dict get $design_data power_nets] {
    set tag "POWER"
    cut_blocked_areas $tag
    add_pad_straps $tag
    generate_grid_vias $tag $pwr_net
  }
  ## Ground nets
  # debug "Ground straps"
  foreach gnd_net [dict get $design_data ground_nets] {
    set tag "GROUND"
    cut_blocked_areas $tag
    add_pad_straps $tag
    generate_grid_vias $tag $gnd_net
  }

  if {[dict exists $grid_data obstructions]} {
    information 32 "Generating blockages for TritonRoute"
    # debug "Obstructions: [dict get $grid_data obstructions]"
    foreach layer_name [dict get $grid_data obstructions] {
      generate_obstructions $layer_name
    }
  }
  # debug "end"
}

proc select_instance_specification {instance} {
  variable design_data
  variable instances
  # debug "start $instance"
  if {[dict exists $design_data grid macro]} {
    set macro_specifications [dict get $design_data grid macro]

    # If there is a specifcation that matches this instance name, use that
    dict for {name specification} $macro_specifications {
      if {![dict exists $specification instance]} {continue}
      if {[dict get $specification instance] == $instance} {
        # debug "instname found, end"
        return $specification
      }
    }
    # If there is a specification that matches this macro name, use that
    if {[dict exists $instances $instance]} {
      set instance_macro [dict get $instances $instance macro]

      # If there are orientation based specifcations for this macro, use the appropriate one if available && [dict get $spec orient]
      dict for {name spec} $macro_specifications {
        if {!([dict exists $spec macro] && [dict exists $spec orient] && [dict get $spec macro] == $instance_macro)} {continue}
        if {[lsearch -exact [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
          # dbug "select_instance_specification: macro orientation found, end"
          return $spec
        }
      }

      # There should only be one macro specific spec that doesnt have an orientation qualifier
      dict for {name spec} $macro_specifications {
        if {!([dict exists $spec macro] && [dict get $spec macro] == $instance_macro)} {continue}
        # debug "macro, no orientation found, end"
        return $spec
      }

      # If there are orientation based specifcations, use the appropriate one if available
      dict for {name spec} $macro_specifications {
        if {!(![dict exists $spec macro] && ![dict exists $spec instance] && [dict exists $spec orient])} {continue}
        if {[lsearch -exact [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
          # debug "other end"
          return $spec
        }
      }
    }

    # There should only be one non-macro specific spec that doesnt have an orientation qualifier
    dict for {name spec} $macro_specifications {
      if {!(![dict exists $spec macro] && ![dict exists $spec instance])} {continue}
      # debug "no macro, no instance, end"
      return $spec
    }

  }

  critical 31 "No matching grid specification found for $instance"
}

proc get_instance_specification {instance} {
  variable instances

  set specification [select_instance_specification $instance]

  if {![dict exists $specification blockages]} {
    dict set specification blockages {}
  }
  dict set specification area [dict get $instances $instance macro_boundary]
  
  return $specification
}

proc get_pitch {layer} {
  if {[$layer hasXYPitch]} {
    if {[get_dir [$layer getName]] == "hor"} {
      return [$layer getPitchY]
    } else {
      return [$layer getPitchX]
    }
  } else {
    return [$layer getPitch]
  }
}

proc init_metal_layers {} {
  variable tech
  variable metal_layers
  variable layers
  variable block
  
  set metal_layers {}        

  foreach layer [$tech getLayers] {
    if {[$layer getType] == "ROUTING"} {
      set_prop_lines $layer LEF58_TYPE
      # Layers that have LEF58_TYPE are not normal ROUTING layers, so should not be considered
      if {![empty_propline]} {continue}

      set layer_name [$layer getName]
      lappend metal_layers $layer_name
      
      if {[$layer getDirection] == "HORIZONTAL"} {
        dict set layers $layer_name direction "hor"
      } else {
        dict set layers $layer_name direction "ver"
      }
      dict set layers $layer_name pitch [get_pitch $layer]

      set tracks [$block findTrackGrid $layer]
      if {$tracks == "NULL"} {
        warning 35 "No track information found for layer $layer_name"
      } else {
        dict set layers $layer_name offsetX [lindex [$tracks getGridX] 0]
        dict set layers $layer_name offsetY [lindex [$tracks getGridY] 0]
      }
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
  if {[dict exists $specification blockages]} {
    return [dict get $specification blockages]
  }
  return $metal_layers
}

proc report_layer_details {layer} {
  variable def_units
  
  set str " - "
  foreach element {width pitch spacing offset core_offset} {
    if {[dict exists $layer $element]} {
      set str [format "$str $element: %.3f " [expr 1.0 * [dict get $layer $element] / $def_units]]
    }
  }
  return $str
}

proc print_strategy {type specification} {
  if {[dict exists $specification name]} {
    puts "Type: ${type}, [dict get $specification name]"
  } else {
    puts "Type: $type"
  }
  if {[dict exists $specification core_ring]} {
    puts "    Core Rings"
    dict for {layer_name layer} [dict get $specification core_ring] {
      puts -nonewline "      Layer: $layer_name"
      if {[dict exists $layer width]} {
        set str [report_layer_details $layer]
        puts $str
      } else {
        puts ""
        foreach template [dict keys $layer] {
          puts -nonewline [format "          %-14s" $template]
          set str [report_layer_details [dict get $layer $template]]
          puts $str
        }
      }
    }
  }
  if {[dict exists $specification rails]} {
    puts "    Stdcell Rails"
    dict for {layer_name layer} [dict get $specification rails] {
      puts -nonewline "      Layer: $layer_name"
      if {[dict exists $layer width]} {
        set str [report_layer_details $layer]
        puts $str
      } else {
        puts ""
        foreach template [dict keys $layer] {
          puts -nonewline [format "          %-14s" $template]
          set str [report_layer_details [dict get $layer $template]]
          puts $str
        }
      }
    }
  }
  if {[dict exists $specification instance]} {
    puts "    Instance: [dict get $specification instance]"
  }
  if {[dict exists $specification macro]} {
    puts "    Macro: [dict get $specification macro]"
  }
  if {[dict exists $specification orient]} {
    puts "    Macro orientation: [dict get $specification orient]"
  }
  if {[dict exists $specification straps]} {
    puts "    Straps"
    dict for {layer_name layer} [dict get $specification straps] {
      puts -nonewline "      Layer: $layer_name"
      if {[dict exists $layer width]} {
        set str [report_layer_details $layer]
        puts $str
      } else {
        puts ""
        foreach template [dict keys $layer] {
          puts -nonewline [format "          %-14s" $template]
          set str [report_layer_details [dict get $layer $template]]
          puts $str
        }
      }
    }
  }
  if {[dict exists $specification connect]} {
    puts "    Connect: [dict get $specification connect]"
  }
}

proc read_template_placement {} {
  variable plan_template
  variable def_units
  variable prop_line

  if {![is_defined_pdn_property "plan_template"]} {
    define_template_grid
  } else {
    set plan_template {}
    set prop_line [get_pdn_string_property_value "plan_template"]
    while {![empty_propline]} {
      set line [read_propline]
      if {[llength $line] == 0} {continue}
      set x  [expr round([lindex $line 0] * $def_units)]
      set y  [expr round([lindex $line 1] * $def_units)]
      set x1 [expr round([lindex $line 2] * $def_units)]
      set y1 [expr round([lindex $line 3] * $def_units)]
      set template [lindex $line end]

      dict set plan_template $x $y $template
    }
  }
}

proc is_defined_pdn_property {name} {
  variable block

  if {[set pdn_props [::odb::dbBoolProperty_find $block PDN]] == "NULL"} {
    return 0
  }
  
  if {[::odb::dbStringProperty_find $pdn_props $name] == "NULL"} {
    return 0
  }
  return 1
}

proc get_pdn_string_property {name} {
  variable block

  if {[set pdn_props [::odb::dbBoolProperty_find $block PDN]] == "NULL"} {
    set pdn_props [::odb::dbBoolProperty_create $block PDN 1]
  }
  
  if {[set prop [::odb::dbStringProperty_find $pdn_props $name]] == "NULL"} {
    set prop [::odb::dbStringProperty_create $pdn_props $name ""]
  }
  
  return $prop
}

proc set_pdn_string_property_value {name value} {
  set prop [get_pdn_string_property $name]
  $prop setValue $value
}

proc get_pdn_string_property_value {name} {
  set prop [get_pdn_string_property $name]

  return [$prop getValue]
}

proc write_template_placement {} {
  variable plan_template
  variable template
  variable def_units
  
  set str ""
  foreach x [lsort -integer [dict keys $plan_template]] {
    foreach y [lsort -integer [dict keys [dict get $plan_template $x]]] {
      set str [format "${str}%.3f %.3f %.3f %.3f %s ;\n" \
        [expr 1.0 * $x / $def_units] [expr 1.0 * $y / $def_units] \
        [expr 1.0 * ($x + [dict get $template width]) / $def_units] [expr 1.0 * ($y + [dict get $template height]) / $def_units] \
        [dict get $plan_template $x $y]
      ]
    }
  }

  set_pdn_string_property_value "plan_template" $str
}

proc get_extent {polygon_set} {
  set first_point  [lindex [odb::getPoints [lindex [odb::getPolygons $polygon_set] 0]] 0]
  set minX [set maxX [$first_point getX]]
  set minY [set maxY [$first_point getY]]

  foreach shape [odb::getPolygons $polygon_set] {
    foreach point [odb::getPoints $shape] {
      set x [$point getX]
      set y [$point getY]
      set minX [expr min($minX,$x)]
      set maxX [expr max($maxX,$x)]
      set minY [expr min($minY,$y)]
      set maxY [expr max($maxY,$y)]
    }
  }

  return [list $minX $minY $maxX $maxY]
}

proc get_stdcell_plus_area {} {
  variable stdcell_area
  variable stdcell_plus_area
  
  if {$stdcell_area == ""} {
    get_stdcell_area
  }
  # debug "stdcell_area      [get_extent $stdcell_area]"
  # debug "stdcell_plus_area [get_extent $stdcell_plus_area]"
  return $stdcell_plus_area
}

proc get_stdcell_area {} {
  variable block
  variable stdcell_area
  variable stdcell_plus_area
  
  if {$stdcell_area != ""} {return $stdcell_area}
  set rails_width [get_rails_max_width]
  
  set rows [$block getRows]
  set first_row [[lindex $rows 0] getBBox]

  set minX [$first_row xMin]
  set maxX [$first_row xMax]
  set minY [$first_row yMin]
  set maxY [$first_row yMax]
  set stdcell_area [odb::newSetFromRect $minX $minY $maxX $maxY]
  set stdcell_plus_area [odb::newSetFromRect $minX [expr $minY - $rails_width / 2] $maxX [expr $maxY + $rails_width / 2]]
    
  foreach row [lrange $rows 1 end] {
    set box [$row getBBox]
    set minX [$box xMin]
    set maxX [$box xMax]
    set minY [$box yMin]
    set maxY [$box yMax]
    set stdcell_area [odb::orSet $stdcell_area [odb::newSetFromRect $minX $minY $maxX $maxY]]
    set stdcell_plus_area [odb::orSet $stdcell_plus_area [odb::newSetFromRect $minX [expr $minY - $rails_width / 2] $maxX [expr $maxY + $rails_width / 2]]]
  }

  return $stdcell_area
}

proc find_core_area {} {
  variable block
  
  set area [get_stdcell_area]

  return [get_extent $area]
}

proc get_rails_max_width {} {
  variable design_data
  variable default_grid_data
  
  set max_width 0
  foreach layer [get_rails_layers] {
     if {[dict exists $default_grid_data rails $layer]} {
       if {[set width [dict get $default_grid_data rails $layer width]] > $max_width} {
         set max_width $width
       }
     }
  }
  
  return $max_width
}

proc core_area_boundary {} {
  variable design_data
  variable template
  variable metal_layers
  variable grid_data
  
  set core_area [find_core_area]
  # We need to allow the rails to extend by half a rails width in the y direction, since the rails overlap the core_area
  
  set llx [lindex $core_area 0] 
  set lly [lindex $core_area 1]
  set urx [lindex $core_area 2]
  set ury [lindex $core_area 3]

  if {[dict exists $template width]} {
    set width [dict get $template width]
  } else {
    set width 2000
  }
  if {[dict exists $template height]} {
    set height [dict get $template height]
  } else {
    set height 2000
  }
  
  # Add blockages around the outside of the core area in order to trim back the templates.
  #
  set boundary [odb::newSetFromRect [expr $llx - $width] [expr $lly - $height] $llx [expr $ury + $height]]
  set boundary [odb::orSet $boundary [odb::newSetFromRect [expr $llx - $width] [expr $lly - $height] [expr $urx + $width] $lly]]
  set boundary [odb::orSet $boundary [odb::newSetFromRect [expr $llx - $width] $ury [expr $urx + $width] [expr $ury + $height]]]
  set boundary [odb::orSet $boundary [odb::newSetFromRect $urx [expr $lly - $height] [expr $urx + $width] [expr $ury + $height]]]
  set boundary [odb::subtractSet $boundary [get_stdcell_plus_area]]
  
  foreach layer $metal_layers {
    if {[dict exists $grid_data core_ring] && [dict exists $grid_data core_ring $layer]} {continue}
    dict set blockages $layer $boundary
  }

  return $blockages
}

proc get_instance_blockages {instances} {
  set blockages {}
  
  foreach inst $instances {
    foreach layer [get_macro_blockage_layers $inst] {
      set box [odb::newSetFromRect [get_instance_llx $inst] [get_instance_lly $inst] [get_instance_urx $inst] [get_instance_ury $inst]]
      if {[dict exists $blockages $layer]} {
        dict set blockages $layer [odb::orSet [dict get $blockages $layer] $box]
      } else {
        dict set blockages $layer $box
      }
    }
  }

  return $blockages
}

proc define_template_grid {} {
  variable design_data
  variable template
  variable plan_template
  variable block 
  variable default_grid_data 
  variable default_template_name
  
  set core_area [dict get $design_data config core_area]
  set llx [lindex $core_area 0]
  set lly [lindex $core_area 1]
  set urx [lindex $core_area 2]
  set ury [lindex $core_area 3]
  
  set core_width  [expr $urx - $llx]
  set core_height [expr $ury - $lly]

  set template_width  [dict get $template width]
  set template_height [dict get $template height]
  set x_sections [expr round($core_width  / $template_width)]
  set y_sections [expr round($core_height / $template_height)]
  
  dict set template offset x [expr [lindex $core_area 0] + ($core_width - $x_sections * $template_width) / 2]
  dict set template offset y [expr [lindex $core_area 1] + ($core_height - $y_sections * $template_height) / 2]
  
  if {$default_template_name == {}} {
    set template_name [lindex [dict get $default_grid_data template names] 0]
  } else {
    set template_name $default_template_name
  }
  
  for {set i -1} {$i <= $x_sections} {incr i} {
    for {set j -1} {$j <= $y_sections} {incr j} {
      set llx [expr $i * $template_width + [dict get $template offset x]]
      set lly [expr $j * $template_height + [dict get $template offset y]]

      dict set plan_template $llx $lly $template_name
    }
  }
  write_template_placement
}

proc set_blockages {these_blockages} {
  variable blockages
  
  set blockages $these_blockages
}
  
proc get_blockages {} {
  variable blockages
  
  return $blockages
}
  
proc add_blockage {layer blockage} {
  variable blockages
  
  if {[dict exists $blockages $layer]} {
    dict set blockages $layer [odb::orSet [dict get $blockages $layer] $blockage]
  } else {
    dict set blockages $layer $blockage
  }
}
  
proc add_blockages {more_blockages} {
  variable blockages
  
  dict for {layer blockage} $more_blockages {
    add_blockage $layer $blockage
  }
}

proc add_macro_based_grids {} {
  variable instances
  variable grid_data
  variable verbose
  
  set_blockages {}
  if {[llength [dict keys $instances]] > 0} {
    information 10 "Inserting macro grid for [llength [dict keys $instances]] macros"
    foreach instance [dict keys $instances] {
      if {$verbose == 1} {
        information 34 "  - grid for instance $instance"
      }
      # debug "$instance [get_instance_specification $instance]"
      set grid_data [get_instance_specification $instance]
      # debug "area=[dict get $grid_data area]"
      variable stripe_locs
      add_grid 
    }
  }
}

proc plan_grid {} {
  variable design_data
  variable instances
  variable default_grid_data
  variable def_units
  variable grid_data
  
  ################################## Main Code #################################

  if {![dict exists $design_data grid stdcell]} {
    warning 17 "No stdcell grid specification found - no rails inserted"
  }

  if {![dict exists $design_data grid macro]} {
    warning 18 "No macro grid specifications found - no straps added"
  }

  information 11 "****** INFO ******"

  if {[dict exists $design_data grid stdcell]} {
    dict for {name specification} [dict get $design_data grid stdcell] {
      print_strategy stdcell $specification
    }
  }

  if {[dict exists $design_data grid macro]} {
    dict for {name specification} [dict get $design_data grid macro] {
      print_strategy macro $specification
    }
  }

  information 12 "**** END INFO ****"

  set specification $default_grid_data
  if {[dict exists $specification name]} {
    information 13 "Inserting stdcell grid - [dict get $specification name]"
  } else {
    information 14 "Inserting stdcell grid"
  }

  if {![dict exists $specification area]} {
    dict set specification area [dict get $design_data config core_area]
  }

  set grid_data $specification

  set_blockages [get_instance_blockages [dict keys $instances]]
  add_blockages [core_area_boundary]        

  if {[dict exists $specification template]} {
    read_template_placement
  }
  
  add_grid

  add_macro_based_grids
}

proc opendb_update_grid {} {
  information 15 "Writing to database"
  export_opendb_vias
  export_opendb_specialnets
}
  
proc apply_pdn {config is_verbose} {
  variable design_data
  variable instances
  variable db
  variable verbose

  set verbose $is_verbose
  
  set db [::ord::get_db]

  set ::start_time [clock clicks -milliseconds]
  if {$verbose} {
    information 16 "Power Delivery Network Generator: Generating PDN"
    information 16 "  config: $config"
  }
  
  apply $config
}

proc apply {config} {
  variable verbose
  
  init $config
  plan_grid

  opendb_update_grid

  if {$verbose} {
#    debug apply "Total walltime to generate PDN DEF = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"
  }
}

}
