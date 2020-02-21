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

sta::define_cmd_args "pdngen" {[-verbose] config_file}

proc pdngen { args } {
  sta::parse_key_args "pdngen" args \
    keys {} flags {-verbose}

  set verbose [info exists flags(-verbose)]

  sta::check_argc_eq1 "pdngen" $args
  set config_file $args
  if { [catch { pdngen::apply_pdn $config_file $verbose } error_msg] } {
    puts $error_msg
  }
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
variable metal_layers_dir {}
variable blockages {} 
variable instances {}
variable default_template_name {}
variable template {}
variable default_cutclass {}

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
  
  set idx [lsearch -exact $metal_layers $layer_name]
  if {[lindex $metal_layers_dir $idx] == "HORIZONTAL"} {
    return "hor"
  } else {
    return "ver"
  }
}

proc get_rails_layers {} {
  variable design_data
  
  foreach type [dict keys [dict get $design_data grid]] {
    dict for {name specification} [dict get $design_data grid $type] {
      if {[dict exists $specification rails]} {
        return [dict keys [dict get $specification rails]]
      }
    }
  }
  error "Cannot determine layer to use for stdcell rails"
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
    error "Cannot find layer $layer_name in loaded technology"
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
  # puts "read_spacing: $layer_name $spacing"
  dict set layers $layer_name spacing $spacing 
  # puts "read_spacing: $layer_name [dict get $layers $layer_name]"
}

proc get_concave_spacing_value {layer_name other_layer_name} {
  variable layers
  variable default_cutclass
  
  if {![dict exists $layers $layer_name spacing]} {
    read_spacing $layer_name
  }
  # puts "get_concave_spacing_value: $layer_name [dict get $layers $layer_name]"
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
  variable tech
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
      error "Failed to read CUTCLASS property '$line'"
    }
    if {$min_area == -1 || [expr $width * $length] < $min_area} {
      dict set default_cutclass $layer_name $cut_class
      set min_area [expr $width * $length]
    }
    dict set layers $layer_name cutclass $cut_class [list width [expr round($width * $def_units)] length [expr round($length * $def_units)]]
  }
}

proc read_enclosures {layer_name} {
  variable tech
  variable layers 
  variable def_units
  
  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_ENCLOSURE
  set prev_cutclass ""

  while {![empty_propline]} {
    set line [read_propline]
    set flags {}
    if {[set idx [lsearch -exact $line EOL]] > -1} {
      # Don't need to handle this rule type
      continue
    }
    if {[set idx [lsearch -exact $line ABOVE]] > -1} {
      dict set flags above 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line BELOW]] > -1} {
      dict set flags below 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line END]] > -1} {
      dict set flags end 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line SIDE]] > -1} {
      dict set flags side 1
      set line [lreplace $line $idx $idx]
    }

    set width 0
    regexp {WIDTH\s+([^\s]+)} $line - width
    set width [expr round($width * $def_units)]
    
    if {![regexp {ENCLOSURE CUTCLASS\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)} $line - cut_class overlap1 overlap2]} {
      error "Failed to read ENCLOSURE property '$line'"
    }
    
    if {$prev_cutclass != $cut_class} {
      set enclosures {}
      set prev_cutclass $cut_class
    }
    dict lappend enclosures $width [list [expr round($overlap1 * $def_units)] [expr round($overlap2 * $def_units)]]
    dict set layers $layer_name cutclass $cut_class enclosures $enclosures 
  }
}

proc get_via_enclosure {via_info width} {
  variable layers
  variable default_cutclass
  
  set layer_name [dict get $via_info cut layer]
  
  if {![dict exists $layers $layer_name cutclass]} {
    read_cutclass $layer_name
    read_enclosures $layer_name
  }

  if {![dict exists $default_cutclass $layer_name]} {
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
    foreach size [lreverse [dict keys $enclosures]] {
      if {$width >= $size} {
          break
      }
    }

    set enclosure [dict get $enclosures $size]
    # puts "get_via_enclosure: enclosure: $enclosure"
    if {[llength $enclosure] > 1} {
      set selected_enclosure [list {*}[lindex $enclosure 0] {*}[lindex $enclosure 0]]
      set selected_min [expr min([lindex $selected_enclosure 0], [lindex $selected_enclosure 1])]
      foreach enc [lrange $enclosure 1 end] {
        set cur_min [expr min([lindex $enc 0], [lindex $enc 1])]
        if {$cur_min < $selected_min} {
          set selected_min $cur_min
          set selected_enclosure [list {*}$enc {*}$enc]
        }
      }
    } else { 
      set selected_enclosure [list {*}[lindex $enclosure 0] {*}[lindex $enclosure 0]]
    }
  }
  # puts "get_via_enclosure: $selected_enclosure"
  return $selected_enclosure

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

proc get_layer_info {} {
  variable layers 
  
  return $layers
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
  if {[lsearch -exact $widthtable $width] > 0} {return $width}
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
  # puts "use_arrayspacing: $arrayspacing"
  # puts "use_arrayspacing: $rows $columns"
  if {[llength $arrayspacing] == 0} {
    # puts "use_arrayspacing: No array spacing rule defined"
    return 0
  }
  # puts "use_arrayspacing: [dict keys [dict get $arrayspacing arraycuts]]"
  if {[dict exists $arrayspacing arraycuts [expr min($rows,$columns)]]} {
    # puts "use_arrayspacing: Matching entry in arrayspacing"
    return 1
  }
  if {[expr min($rows,$columns)] < [lindex [dict keys [dict get $arrayspacing arraycuts]] 0]} {
    # puts "use_arrayspacing: row/columns less than min array spacing"
    return 0
  }
  if {[expr min($rows,$columns)] > [lindex [dict keys [dict get $arrayspacing arraycuts]] end]} {
    # puts "use_arrayspacing: row/columns greater than min array spacing"
    return 1
  }
  # puts "use_arrayspacing: default 1"
  return 1
}

# Given the via rule expressed in via_info, what is the via with the largest cut area that we can make
# Try using a via generate rule
proc get_via_option {lower width height constraints} {
  variable def_units
  
# puts "get_via_option {$lower $width $height}"
  set via_info [lindex [select_via_info $lower] 1]
  set lower_dir [get_dir $lower]

  set upper [dict get $via_info upper layer]
  
  set cut_width  [lindex [dict get $via_info cut size] 0]
  set cut_height [lindex [dict get $via_info cut size] 1]

  # Adjust the width and height values to the next largest allowed value if necessary
  set lower_width  [get_adjusted_width $lower $width]
  set lower_height [get_adjusted_width $lower $height]
  set upper_width  [get_adjusted_width $upper $width]
  set upper_height [get_adjusted_width $upper $height]
  
  set via_enclosure [get_via_enclosure $via_info [expr min($lower_width,$lower_height,$upper_width,$upper_height)]]
  set min_lower_enclosure [lindex $via_enclosure 0]
  set max_lower_enclosure [lindex $via_enclosure 1]
  set min_upper_enclosure [lindex $via_enclosure 2]
  set max_upper_enclosure [lindex $via_enclosure 3]

  # What are the maximum number of rows and columns that we can fit in this space?
  set i 0
  set via_width_lower 0
  set via_width_upper 0
  set xcut_pitch [lindex [dict get $via_info cut spacing] 0]
  if {[dict exists $constraints cut_pitch]} {set xcut_pitch [expr round([dict get $constraints cut_pitch] * $def_units)]}
  
  while {$via_width_lower < $lower_width && $via_width_upper < $upper_width} {
    # puts "get_via_option: W: $via_width_lower < $lower_width && $via_width_upper < $upper_width"
    incr i
    set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
    set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
  }
  set xcut_spacing [expr $xcut_pitch - $cut_width]
  set columns [expr $i - 1]
  if {[dict exists $constraints max_columns]} {
    if {$columns > [dict get $constraints max_columns]} {
      set columns [dict get $constraints max_columns]

      set lower_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] $lower]
      # puts "get_via_option: $lower_concave_enclosure $max_lower_enclosure"
      if {$lower_concave_enclosure > $max_lower_enclosure} {
        set max_lower_enclosure $lower_concave_enclosure
      }
      set upper_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] $upper]
      # puts "get_via_option: $upper_concave_enclosure $max_upper_enclosure"
      if {$upper_concave_enclosure > $max_upper_enclosure} {
        set max_upper_enclosure $upper_concave_enclosure
      }

      set lower_width [expr $cut_width + $xcut_pitch * ($columns - 1) + 2 * $max_lower_enclosure]
      set upper_width [expr $cut_width + $xcut_pitch * ($columns - 1) + 2 * $max_upper_enclosure]
      set lower_width [get_adjusted_width [dict get $via_info lower layer] $lower_width]
      set upper_width [get_adjusted_width [dict get $via_info lower layer] $upper_width]
    }
  }
  set i 0
  set via_height_lower 0
  set via_height_upper 0
  set ycut_pitch [lindex [dict get $via_info cut spacing] 1]
  if {[dict exists $constraints cut_pitch]} {set ycut_pitch [expr round([dict get $constraints cut_pitch] * $def_units)]}
  while {$via_height_lower < $lower_height && $via_height_upper < $upper_height} {
    # puts "get_via_option: H: $via_height_lower < $lower_height && $via_height_upper < $upper_height"
    incr i
    set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
    set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
  }
  set ycut_spacing [expr $ycut_pitch - $cut_height]
  set rows [expr $i - 1]
  if {[dict exists $constraints max_rows]} {
    if {$columns > [dict get $constraints max_columns]} {
      set columns [dict get $constraints max_columns]

      set lower_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] $lower]
      # puts "get_via_option: $lower_concave_enclosure $max_lower_enclosure"
      if {$lower_concave_enclosure > $max_lower_enclosure} {
        set max_lower_enclosure $lower_concave_enclosure
      }
      set upper_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] $upper]
      # puts "get_via_option: $upper_concave_enclosure $max_upper_enclosure"
      if {$upper_concave_enclosure > $max_upper_enclosure} {
        set max_upper_enclosure $upper_concave_enclosure
      }

      set lower_height [expr $cut_height + $ycut_pitch * ($rows - 1) + 2 * $max_lower_enclosure]
      set upper_height [expr $cut_height + $ycut_pitch * ($rows - 1) + 2 * $max_upper_enclosure]
      set lower_height [get_adjusted_width [dict get $via_info lower layer] $lower_height]
      set upper_height [get_adjusted_width [dict get $via_info lower layer] $upper_height]
    }                                                                              
  }

  if {![use_arrayspacing [dict get $via_info cut layer] $rows $columns]} {
    set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
    set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]
    set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
    set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]

    # Adjust calculated via width values to ensure that an allowed size is generated
    set lower_size_max_enclosure [get_adjusted_width $lower [expr round(($cut_width   + $xcut_pitch * ($columns - 1) + $max_lower_enclosure * 2))]]
    set upper_size_max_enclosure [get_adjusted_width $upper [expr round(($cut_width   + $xcut_pitch * ($columns - 1) + $max_upper_enclosure * 2))]]

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
    if {[get_dir $upper] == "hor"} {
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
      name [get_viarule_name $lower $width $height] \
      rule [lindex [select_via_info $lower] 0] \
      cutsize [dict get $via_info cut size] \
      layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] $upper] \
      cutspacing [list $xcut_spacing $ycut_spacing] \
      rowcol [list $rows $columns] \
      enclosure [list $xBotEnc $yBotEnc $xTopEnc $yTopEnc] \
      origin_x 0 origin_y 0]
    set rule_list [list $rule]
  } else {
    # We need array vias -
    # if the min(rows,columns) > ARRAYCUTS
    #   determine which direction gives best number of CUTs wide using min(ARRAYCUTS)
    #   After adding ARRAYs, is there space for more vias
    #   Add vias to the rule with appropriate origin setting
    # else
    #   add a single via with min(rows,columns) cuts - hor/ver as required
    set spacing_rule [get_arrayspacing_rule [dict get $via_info cut layer]]
    set array_size [expr min($rows, $columns)]
    if {$array_size > [lindex [dict keys [dict get $spacing_rule arraycuts]] end]} {
      # puts "get_via_option: Multi-viaArrayspacing rule"
      set use_array_size [lindex [dict keys [dict get $spacing_rule arraycuts]] 0]
      foreach other_array_size [lrange [dict keys [dict get $spacing_rule arraycuts]] 1 end] {
        if {$array_size % $use_array_size > $array_size % $other_array_size} {
          set use_array_size $other_array_size
        }
      }
      set num_arrays [expr $array_size / $use_array_size]
      set array_spacing [expr max($xcut_spacing,$ycut_spacing,[dict get $spacing_rule arraycuts $use_array_size spacing])]
      
      set rule [list \
        rule [lindex [select_via_info $lower] 0] \
        cutsize [dict get $via_info cut size] \
        layers [list $lower [dict get $via_info cut layer] $upper] \
        cutspacing [list $xcut_spacing $ycut_spacing] \
        enclosure $via_enclosure \
        origin_x 0 \
        origin_y 0 \
      ]
      # puts "get_via_option: $rule"
      set rule_list {}
      if {$array_size == $rows} {
        # Split into num_arrays rows of arrays
        set array_min_size [expr [lindex [dict get $via_info cut size] 0] * $use_array_size + [dict get $spacing_rule cutspacing] * ($use_array_size - 1)]
        set total_array_size [expr $array_min_size * $num_arrays + $array_spacing * ($num_arrays - 1)]

        dict set rule rowcol [list $use_array_size $columns]
        dict set rule name "[dict get $via_info cut layer]_ARRAY_${use_array_size}X${columns}"

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

        dict set rule rowcol [list $rows $use_array_size]
        dict set rule name "[dict get $via_info cut layer]_ARRAY_${rows}X${use_array_size}"

        set x [expr $array_min_size / 2 - $total_array_size / 2]
        for {set i 0} {$i < $num_arrays} {incr i} {
          dict set rule origin_x $x
          lappend rule_list $rule
          set x [expr $x + $array_spacing + $array_min_size]
        }
      }
    } else {
      # puts "get_via_option: Arrayspacing rule"
      set rule [list \
        name [get_viarule_name $lower $width $height] \
        rule [lindex [select_via_info $lower] 0] \
        cutsize [dict get $via_info cut size] \
        layers [list $lower [dict get $via_info cut layer] $upper] \
        cutspacing [list $xcut_spacing $ycut_spacing] \
        rowcol [list $rows $columns] \
        enclosure $via_enclosure \
        origin_x 0 \
        origin_y 0 \
      ]
      set rule_list [list $rule]
    }
  }
  
  return $rule_list
}

proc get_viarule_name {lower width height} {
  set rules [select_via_info $lower]
  set first_key [lindex [dict keys $rules] 0]
  #if {![dict exists $rules $first_key cut layer]} {
  #  puts "get_viarule_name: $lower $width $height"
  #  puts "get_viarule_name: $rules"
  #  puts "get_viarule_name: $first_key"
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
    # puts "get_via: Via [dict size $physical_viarules]: $rule_name"
  }

  return $rule_name
}

proc instantiate_via {physical_via_name x y} {
  variable physical_viarules
  
  set via_insts {}
  foreach via [dict get $physical_viarules $physical_via_name] {
    dict set via x [expr $x + [dict get $via origin_x]]
    dict set via y [expr $y + [dict get $via origin_y]]
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
  if {$i1 == -1} {error "Cant find layer $layer1"}
  if {$i2 == -1} {error "Cant find layer $layer2"}

  # For each layer between l1 and l2, add vias at the intersection
  # puts "  # Intersections [llength $intersections]"
  set count 0
  foreach intersection $intersections {
    if {![dict exists $logical_viarules [dict get $intersection rule]]} {
      error "Missing key [dict get $intersection rule]\nAvailable keys [dict keys $logical_viarules]"
    }
    set logical_rule [dict get $logical_viarules [dict get $intersection rule]]

    set x [dict get $intersection x]
    set y [dict get $intersection y]
    set width  [dict get $logical_rule width]
    set height  [dict get $logical_rule height]
    
    set connection_layers [lrange $metal_layers $i1 [expr $i2 - 1]]
    # puts "  # Connection layers: [llength $connection_layers]"
    # puts "  Connection layers: $connection_layers"
    foreach lay $connection_layers {
      set via_name [get_via $lay $width $height $constraints]
      foreach via [instantiate_via $via_name $x $y] {
        lappend vias $via
      }
    }
    
    incr count
    #if {$count % 1000 == 0} {
    #  puts "  # $count / [llength $intersections]"
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
proc generate_via_stacks {l1 l2 tag grid_data constraints} {
  variable logical_viarules
  variable stripe_locs
  variable def_units
  variable metal_layers
  variable blockages
  
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
    puts "No shapes on layer $l1 for $tag"
    return {}
  }
  if {[array names stripe_locs "$l2,$tag"] == ""} {
    puts "No shapes on layer $l2 for $tag"
    return {}
  }
  set intersection [odb::odb_andSet $stripe_locs($l1,$tag) $stripe_locs($l2,$tag)]

  foreach shape [::odb::odb_getPolygons $intersection] {
    set points [::odb::odb_getPoints $shape]
    if {[llength $points] != 4} {
      error "unexpected number of points in shape ([llength $points])"
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
  
  # puts "Added [llength $intersections] intersections"

  return [generate_vias $l1 $l2 $intersections $constraints]
}

# proc to generate follow pin layers or standard cell rails
proc generate_lower_metal_followpin_rails {grid_data} {
  variable stripe_locs
  variable block
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
        error "unexpected row orientation $orient for row [$row getName]"
      }
    }
    foreach lay [get_rails_layers] {
      set width [dict get $grid_data rails $lay width]
      set vdd_box [::odb::odb_newSetFromRect [$box xMin] [expr $vdd_y - $width / 2] [$box xMax] [expr $vdd_y + $width / 2]]
      set vss_box [::odb::odb_newSetFromRect [$box xMin] [expr $vss_y - $width / 2] [$box xMax] [expr $vss_y + $width / 2]]
      # puts "generate_lower_metal_followpin_rails: [$box xMin] [expr $vdd_y - $width / 2] [$box xMax] [expr $vdd_y + $width / 2]"
      if {[array names stripe_locs "$lay,POWER"] == ""} {
        set stripe_locs($lay,POWER) $vdd_box
        set stripe_locs($lay,GROUND) $vss_box
      } else {
        set stripe_locs($lay,POWER)  [::odb::odb_orSet $stripe_locs($lay,POWER)  $vdd_box]
        set stripe_locs($lay,GROUND) [::odb::odb_orSet $stripe_locs($lay,GROUND) $vss_box]
      }
    }
  }
}


# proc for creating pdn mesh for upper metal layers
proc generate_upper_metal_mesh_stripes {tag layer layer_info area} {
  variable stripe_locs
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
      set box [::odb::odb_newSetFromRect [lindex $area 0] [expr $y - $width / 2] [lindex $area 2] [expr $y + $width / 2]]
      if {[array names stripe_locs "$layer,$tag"] == ""} {
        set stripe_locs($layer,$tag) $box
      } else {
        set stripe_locs($layer,$tag) [::odb::odb_orSet $stripe_locs($layer,$tag) $box]
      }
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
      set box [::odb::odb_newSetFromRect [expr $x - $width / 2] [lindex $area 1] [expr $x + $width / 2] [lindex $area 3]]
      if {[array names stripe_locs "$layer,$tag"] == ""} {
        set stripe_locs($layer,$tag) $box
      } else {
        set stripe_locs($layer,$tag) [::odb::odb_orSet $stripe_locs($layer,$tag) $box]
      }
    }
  } else {
    error "Invalid direction \"[get_dir $layer]\" for metal layer ${layer}. Should be either \"hor\" or \"ver\"."
  }
}

## this is a top-level proc to generate PDN stripes and insert vias between these stripes
proc generate_stripes_vias {tag net_name grid_data} {
  variable vias
  variable blockages
  variable plan_template
  variable template
  variable stripe_locs

  foreach lay [dict keys [dict get $grid_data straps]] {
    # puts "    Layer $lay ..."

    #Upper layer stripes
    if {[dict exists $grid_data straps $lay width]} {
      set area [dict get $grid_data area]
      generate_upper_metal_mesh_stripes $tag $lay [dict get $grid_data straps $lay] $area
      set width [dict get $grid_data straps $lay width]
    } else {
      foreach x [lsort -integer [dict keys $plan_template]] {
        foreach y [lsort -integer [dict keys [dict get $plan_template $x]]] {
          set template_name [dict get $plan_template $x $y]
          set layer_info [dict get $grid_data straps $lay $template_name]
          set area [list $x $y [expr $x + [dict get $template width]] [expr $y + [dict get $template height]]]
          # puts "generate_stripes_vias: Adding straps for $area"
          generate_upper_metal_mesh_stripes $tag $lay $layer_info $area
        }
      }
      set width [dict get $layer_info width]
    }
    if {[dict exists $blockages $lay]} {
      set stripe_locs($lay,$tag) [::odb::odb_subtractSet $stripe_locs($lay,$tag) [dict get $blockages $lay]]
      # Trim any shapes that are less than the width of the wire
      set size_by [expr $width / 2 - 1]
      set trimmed_set [::odb::odb_shrinkSet $stripe_locs($lay,$tag) $size_by]
      set stripe_locs($lay,$tag) [::odb::odb_bloatSet $trimmed_set $size_by]
    }
  }

  #Via stacks
  if {[dict exists $grid_data connect]} {
    # puts "Adding vias for $net_name ([llength [dict get $grid_data connect]] connections)..."
    foreach connection [dict get $grid_data connect] {
        set l1 [lindex $connection 0]
        set l2 [lindex $connection 1]
        # puts "    $l1 to $l2"
        set constraints {}
        if {[dict exists $connection constraints]} {
          set constraints [dict get $connection constraints]
        }
        # puts "    Constraints: $constraints"
        set connections [generate_via_stacks $l1 $l2 $tag $grid_data $constraints]
        lappend vias [list net_name $net_name connections $connections]
    }
  }
  # puts "End"
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

  # puts "import_macro_boundaries: start"
  set macros {}
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

  set instances [import_def_components [dict keys $macros]]

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
  
  # puts "import_macro_boundaries: end"
  
}

proc import_def_components {macros} {
  variable design_data
  variable block
  set instances {}

  foreach inst [$block getInsts] {
    set macro_name [[$inst getMaster] getName]
    if {[lsearch -exact $macros $macro_name] != -1} {
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

proc export_opendb_vias {} {
  variable physical_viarules
  variable block
  variable tech
  # puts "export_opendb_vias: [llength $physical_viarules]"
  dict for {name rules} $physical_viarules {
    foreach rule $rules {
      # puts "export_opendb_vias: $rule"
      set via [$block findVia [dict get $rule name]]
      if {$via == "NULL"} {
        set via [odb::dbVia_create $block [dict get $rule name]]
        # puts "export_opendb_vias: Via $via"

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
  # puts "export_opendb_vias: end"
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

  # puts "export_opendb_specialnet: layers - $metal_layers"
  foreach lay $metal_layers {
    if {[array names stripe_locs "$lay,$signal_type"] == ""} {continue}

    set layer [$tech findLayer $lay]

    foreach shape [::odb::odb_getPolygons $stripe_locs($lay,$signal_type)] {
      set points [::odb::odb_getPoints $shape]
      if {[llength $points] != 4} {
        variable def_units
        puts "unexpected number of points in shape ($lay $signal_type [llength $points])"
        puts -nonewline "    "
        foreach point $points {puts -nonewline "([expr 1.0 * [$point getX] / $def_units ] [expr 1.0 * [$point getY] / $def_units]) "}
        puts ""
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
      # puts "export_opendb_specialnet: $xMin $yMin $xMax $yMax $wire_type"
      odb::dbSBox_create $swire $layer $xMin $yMin $xMax $yMax $wire_type
    }
  }
  variable vias
  # puts "export_opendb_specialnet: vias - [llength $vias]"
  foreach via $vias {
    if {[dict get $via net_name] == $net_name} {
      # For each layer between l1 and l2, add vias at the intersection
      foreach via_inst [dict get $via connections] {
        # puts "export_opendb_specialnet: $via_inst"
        set via_name [dict get $via_inst name]
        set x        [dict get $via_inst x]
        set y        [dict get $via_inst y]
        # puts "export_opendb_specialnet: $via_name $x $y [$block findVia $via_name]"
        odb::dbSBox_create $swire [$block findVia $via_name] $x $y "STRIPE"
        # puts "export_opendb_specialnet: via created"
      }
    }
  }
  # puts "export_opendb_specialnet: end"
}
  
proc export_opendb_specialnets {} {
  variable block
  variable design_data
  
  foreach net_name [dict get $design_data power_nets] {
    export_opendb_specialnet "VDD" "POWER"
  }

  foreach net_name [dict get $design_data ground_nets] {
    export_opendb_specialnet "VSS" "GROUND"
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
    default {error "Illegal orientation $orientation specified"}
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
  variable stripe_locs

  # puts "get_memory_instance_pg_pins: start"

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

    # puts "get_memory_instance_pg_pins: cell name - [$master getName]"

    foreach term_name [concat [get_macro_power_pins $inst_name] [get_macro_ground_pins $inst_name]] {
      set inst_term [$inst findITerm $term_name]
      if {$inst_term == "NULL"} {continue}
      
      set mterm [$inst_term getMTerm]
      set type [$mterm getSigType]
      set pin_shapes ""
      foreach mPin [$mterm getMPins] {
        foreach geom [$mPin getGeometry] {
          set layer [[$geom getTechLayer] getName]
          set box [transform_box [$geom xMin] [$geom yMin] [$geom xMax] [$geom yMax] [$inst getOrigin] [$inst getOrient]]

          set width  [expr abs([lindex $box 2] - [lindex $box 0])]
          set height [expr abs([lindex $box 3] - [lindex $box 1])]

          if {$width > $height} {
            set layer_name ${layer}_PIN_hor
          } else {
            set layer_name ${layer}_PIN_ver
          }
          set pin_shape [odb::odb_newSetFromRect {*}$box]

          if {$pin_shapes == ""} {
            set pin_shapes $pin_shape
          } else {
            set pin_shapes [odb::odb_orSet $pin_shapes $pin_shape]
          }            
        }
      }
      if {[array names stripe_locs "$layer_name,$type"] == ""} {
        set stripe_locs($layer_name,$type) $pin_shapes
      } else {
        set stripe_locs($layer_name,$type) [odb::odb_orSet $stripe_locs($layer_name,$type) $pin_shapes]
      }
    }    
  }
#    puts "Total walltime till macro pin geometry creation = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"
  # puts "get_memory_instance_pg_pins: end"
}

proc set_core_area {xmin ymin xmax ymax} {
  variable design_data

  dict set design_data config core_area [list $xmin $ymin $xmax $ymax]
}

proc write_pdn_strategy {} {
  variable design_data
  
  set_pdn_string_property_value "strategy" [dict get $design_data grid]
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
    error "File $PDN_cfg does not exist, or exists but empty"
  }

  set tech [$db getTech]
  set libs [$db getLibs]
  set block [[$db getChip] getBlock]
  set def_units [$block getDefUnits]
  set design_name [$block getName]

  set design_data {}
  set physical_viarules {}
  set stdcell_area ""
  
  source $PDN_cfg
  write_pdn_strategy 
  
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

  ########################################
  # Creating blockages based on macro locations
  #######################################
  import_macro_boundaries

  get_memory_instance_pg_pins

  set default_grid_data [dict get $design_data grid stdcell [lindex [dict keys [dict get $design_data grid stdcell]] 0]]

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
  
  ##### Basic sanity checks to see if inputs are given correctly
  foreach layer [get_rails_layers] {
    if {[lsearch -exact $metal_layers $layer] < 0} {
      error "ERROR: Layer specified for std. cell rails '$layer' not in list of layers."
    }
  }

#        puts "Total walltime till PDN setup = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"

  return $design_data
}

proc convert_layer_spec_to_def_units {data} {
  variable def_units
  
  foreach key {width pitch spacing offset} {
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
  if {[dict exists $specification template]} {
    set_template_size {*}[dict get $specification template size]
  }
  
  dict set design_data grid $type $spec_name $spec
}

proc add_grid {grid_data} {
  variable design_data
  
  #puts "Adding stripes for $net_name ..."
  if {[dict exists $grid_data rails]} {
    set area [dict get $grid_data area]
    generate_lower_metal_followpin_rails $grid_data
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

  if {[dict exists $design_data grid macro]} {
    set macro_specifications [dict get $design_data grid macro]

    # If there is a specifcation that matches this instance name, use that
    dict for {name specification} $macro_specifications {
      if {![dict exists $specification instance]} {continue}
      if {[dict get $specification instance] == $instance} {
        return $specification
      }
    }
    # If there is a specification that matches this macro name, use that
    if {[dict exists $instances $instance]} {
      set instance_macro [dict get $instances $instance macro]

      # If there are orientation based specifcations for this macro, use the appropriate one if available
      dict for {name spec} $macro_specifications {
        if {!([dict exists $spec macro] && [dict get $spec orient] && [dict get $spec macro] == $instance_macro)} {continue}
        if {[lsearch -exact [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
          return $spec
        }
      }

      # There should only be one macro specific spec that doesnt have an orientation qualifier
      dict for {name spec} $macro_specifications {
        if {!([dict exists $spec macro] && [dict get $spec macro] == $instance_macro)} {continue}
        if {[lsearch -exact [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
          return $spec
        }
      }

      # If there are orientation based specifcations, use the appropriate one if available
      dict for {name spec} $macro_specifications {
        if {!(![dict exists $spec macro] && ![dict exists $spec instance] && [dict exists $spec orient])} {continue}
        if {[lsearch -exact [dict get $spec orient] [dict get $instances $instance orient]] != -1} {
          return $spec
        }
      }
    }

    # There should only be one macro specific spec that doesnt have an orientation qualifier
    dict for {name spec} $macro_specifications {
      if {!(![dict exists $spec macro] && ![dict exists $spec instance])} {continue}
      return $spec
    }

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
      set_prop_lines $layer LEF58_TYPE
      # Layers that have LEF58_TYPE are not normal ROUTING layers, so should not be considered
      if {![empty_propline]} {continue}
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
  if {[dict exists $specification blockages]} {
    return [dict get $specification blockages]
  }
  return $metal_layers
}

proc print_layer_details {layer_name layer indent} {
  variable def_units
  
   if {[dict exists $layer spacing]} {
     puts [format " - Width: %.3f Pitch: %.3f Spacing: %.3f Offset: %.3f" \
       [expr 1.0 * [dict get $layer width] / $def_units] \
       [expr 1.0 * [dict get $layer pitch] / $def_units] \
       [expr 1.0 * [dict get $layer spacing] / $def_units] \
       [expr 1.0 * [dict get $layer offset] / $def_units]]
   } else {
     puts [format " - Width: %.3f Pitch: %.3f Offset: %.3f" \
       [expr 1.0 * [dict get $layer width] / $def_units] \
       [expr 1.0 * [dict get $layer pitch] / $def_units] \
       [expr 1.0 * [dict get $layer offset] / $def_units]]
   }
}

proc print_strategy {type specification} {
  if {[dict exists $specification name]} {
    puts "Type: ${type}, [dict get $specification name]"
  } else {
    puts "Type: $type"
  }
  if {[dict exists $specification rails]} {
    puts "    Stdcell Rails"
    dict for {layer_name layer} [dict get $specification rails] {
      puts -nonewline "      Layer: $layer_name"
      if {[dict exists $layer width]} {
        print_layer_details $layer_name $layer 4
      } else {
        puts ""
        foreach template [dict keys $layer] {
          puts -nonewline [format "          %-14s" $template]
          print_layer_details $layer_name [dict get $layer $template] 8
        }
      }
    }
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
  if {[dict exists $specification straps]} {
    puts "    Straps"
    dict for {layer_name layer} [dict get $specification straps] {
      puts -nonewline "      Layer: $layer_name"
      if {[dict exists $layer width]} {
        print_layer_details $layer_name $layer 4
      } else {
        puts ""
        foreach template [dict keys $layer] {
          puts -nonewline [format "          %-14s" $template]
          print_layer_details $layer_name [dict get $layer $template] 8
        }
      }
    }
  }
  puts "    Connect: [dict get $specification connect]"
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

proc set_default_template {name} {
  variable design_data
  
  dict set design_data config default_template_name $name
}

proc get_specification_template {x y} {
  variable default_grid_data
}

proc get_extent {polygon_set} {
  set first_point  [lindex [odb::odb_getPoints [lindex [odb::odb_getPolygons $polygon_set] 0]] 0]
  set minX [set maxX [$first_point getX]]
  set minY [set maxY [$first_point getY]]

  foreach shape [odb::odb_getPolygons $polygon_set] {
    foreach point [odb::odb_getPoints $shape] {
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
  # puts "get_stdcell_plus_area: stdcell_area      [get_extent $stdcell_area]"
  # puts "get_stdcell_plus_area: stdcell_plus_area [get_extent $stdcell_plus_area]"
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
  set stdcell_area [odb::odb_newSetFromRect $minX $minY $maxX $maxY]
  set stdcell_plus_area [odb::odb_newSetFromRect $minX [expr $minY - $rails_width / 2] $maxX [expr $maxY + $rails_width / 2]]
    
  foreach row [lrange $rows 1 end] {
    set box [$row getBBox]
    set minX [$box xMin]
    set maxX [$box xMax]
    set minY [$box yMin]
    set maxY [$box yMax]
    set stdcell_area [odb::odb_orSet $stdcell_area [odb::odb_newSetFromRect $minX $minY $maxX $maxY]]
    set stdcell_plus_area [odb::odb_orSet $stdcell_plus_area [odb::odb_newSetFromRect $minX [expr $minY - $rails_width / 2] $maxX [expr $maxY + $rails_width / 2]]]
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
  set boundary [odb::odb_newSetFromRect [expr $llx - $width] [expr $lly - $height] $llx [expr $ury + $height]]
  set boundary [odb::odb_orSet $boundary [odb::odb_newSetFromRect [expr $llx - $width] [expr $lly - $height] [expr $urx + $width] $lly]]
  set boundary [odb::odb_orSet $boundary [odb::odb_newSetFromRect [expr $llx - $width] $ury [expr $urx + $width] [expr $ury + $height]]]
  set boundary [odb::odb_orSet $boundary [odb::odb_newSetFromRect $urx [expr $lly - $height] [expr $urx + $width] [expr $ury + $height]]]
  set boundary [odb::odb_subtractSet $boundary [get_stdcell_plus_area]]
  
  foreach layer $metal_layers {
    dict set blockages $layer $boundary
  }

  return $blockages
}

proc get_instance_blockages {instances} {
  variable metal_layers

  set blockages {}
  
  foreach inst $instances {
    foreach layer [get_macro_blockage_layers $inst] {
      set box [odb::odb_newSetFromRect [get_instance_llx $inst] [get_instance_lly $inst] [get_instance_urx $inst] [get_instance_ury $inst]]
      if {[dict exists $blockages $layer]} {
        dict set blockages $layer [odb::odb_orSet [dict get $blockages $layer] $box]
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
  
proc add_blockages {more_blockages} {
  variable blockages
  
  dict for {layer blockage} $more_blockages {
    if {[dict exists $blockages $layer]} {
      dict set blockages $layer [odb::odb_orSet [dict get $blockages $layer] $blockage]
    } else {
      dict set blockages $layer $blockage
    }
  }
}

proc add_macro_based_grids {} {
  variable instances

  set_blockages {}
  if {[llength [dict keys $instances]] > 0} {
    puts "Inserting macro grid for [llength [dict keys $instances]] macros"
    foreach instance [dict keys $instances] {
      add_grid [get_instance_specification $instance]
    }
  }
}

proc plan_grid {} {
  variable design_data
  variable instances
  variable default_grid_data
  variable def_units
  variable blockages 
  
  ################################## Main Code #################################

  puts "****** INFO ******"
  dict for {name specification} [dict get $design_data grid stdcell] {
    print_strategy stdcell $specification
  }
  dict for {name specification} [dict get $design_data grid macro] {
    print_strategy macro $specification
  }
  puts "**** END INFO ****"

  set specification $default_grid_data
  if {[dict exists $specification name]} {
    puts "Inserting stdcell grid - [dict get $specification name]"
  } else {
    puts "Inserting stdcell grid"
  }

  dict set specification blockage $blockages
  if {![dict exists $specification area]} {
    dict set specification area [dict get $design_data config core_area]
  }

  set_blockages [get_instance_blockages [dict keys $instances]]
  add_blockages [core_area_boundary]        

  if {[dict exists $specification template]} {
    read_template_placement
  }
  
  add_grid $specification

  add_macro_based_grids
}

proc opendb_update_grid {} {
  puts "Writing to database"
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
    puts "##Power Delivery Network Generator: Generating PDN"
    puts "##  config: $config"
  }
  
  apply $config
}

proc apply {config} {
  variable verbose
  
  init $config
  plan_grid

  opendb_update_grid

  if {$verbose} {
#    puts "Total walltime to generate PDN DEF = [expr {[expr {[clock clicks -milliseconds] - $::start_time}]/1000.0}] seconds"
  }
}

}
