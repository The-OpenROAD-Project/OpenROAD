sta::define_cmd_args "create_physical_cluster" {cluster_name}

proc create_physical_cluster { args } {
  sta::parse_key_args "create_physical_cluster" args keys {} flags {}
  sta::check_argc_eq1 "create_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 308 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [odb::dbGroup_create $block $cluster_name]
  if { $group == "NULL" } {
    utl::error ODB 309 "duplicate group name"
  }
}

sta::define_cmd_args "create_child_physical_clusters" {[-top_module | -modinst path]}

proc create_child_physical_clusters { args } {
  sta::parse_key_args "create_child_physical_clusters" args keys {-modinst} flags {-top_module}
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 310 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  if { [info exists flags(-top_module)] } {
    set module [$block getTopModule]
  } elseif { [info exists keys(-modinst)] } {
    set module [[$block findModInst $keys(-modinst)] getMaster]
  } else {
    utl::error ODB 311 "please define either top module or the modinst path"
  }
  if { $module == "NULL" } {
    utl::error ODB 312 "module does not exist"
  }
  set module_instance [$module getModInst]
  set modinsts [$module getChildren]
  set insts [$module getInsts]
  foreach modinst $modinsts {
    set cluster_name "[$module getName]_[$modinst getName]"
    set group [odb::dbGroup_create $block $cluster_name]
    if { $group == "NULL" } {
      utl::error ODB 313 "duplicate group name"
    }
    $group addModInst $modinst
  }
  if { [llength $insts] > 0 } {
    if { $module_instance == "NULL" } {
      set group [odb::dbGroup_create $block "[$module getName]_glue"]
    } else {
      set parent [$module_instance getParent]
      set group [odb::dbGroup_create $block "[$parent getName]_[$module_instance getName]_glue"]
    }
    if { $group == "NULL" } {
      utl::error ODB 314 "duplicate group name"
    }
    foreach inst $insts {
      $group addInst $inst
    }
  }
}

sta::define_cmd_args "set_ndr_layer_rule" {tech ndr layerName input isSpacing};# checker off

proc set_ndr_layer_rule { tech ndr layerName input isSpacing} {
  set layer [$tech findLayer $layerName]
  if { $layer == "NULL" } {
    utl::warn ODB 1000 "Layer ${layerName} not found, skipping NDR for this layer"
    return
  }
  if { [$layer getType] != "ROUTING" } {
    return
  }
  set rule [$ndr getLayerRule $layer]
  if { $rule == "NULL" } {
    set rule [odb::dbTechLayerRule_create $ndr $layer]
  }
  set input [string trim $input]
  if { [string is double $input] } {
    set value [ord::microns_to_dbu $input]
  } elseif { [string first "*" $input] == 0 } {
    if { $isSpacing } {
      set value [expr [string trim $input "*"] * [$layer getSpacing]]
    } else {
      set value [expr [string trim $input "*"] * [$layer getWidth]]
    }
  } else {
    utl::warn ODB 1009 "Invalid input in create_ndr cmd"
    return
  }
  if { $isSpacing } {
    $rule setSpacing $value
  } else {
    $rule setWidth $value
  }
}

sta::define_cmd_args "set_ndr_rules" {tech ndr values isSpacing};#checker off

proc set_ndr_rules { tech ndr values isSpacing } {
  if { [llength $values] == 1 } {
    # Omitting layers
    set value [lindex $values 0]
    foreach layer [$tech getLayers] {
      if { [$layer getType] != "ROUTING" } {
        continue
      }
      set_ndr_layer_rule $tech $ndr [$layer getName] $value $isSpacing
    }
    return
  }
  for {set i 0} {$i < [llength $values]} {incr i 2} {
    set layers [lindex $values $i]
    set value [lindex $values [expr $i + 1]]
    if { [string first ":" $layers] == -1 } {
      set_ndr_layer_rule $tech $ndr $layers $value $isSpacing
    } else {
      lassign [split $layers ":" ] firstLayer lastLayer
      set foundFirst 0
      set foundLast 0
      foreach layer [$tech getLayers] {
        if { [$layer getType] != "ROUTING" } {
          continue
        }
        if { $foundFirst == 0 } {
          if { [$layer getName] == $firstLayer } {
            set foundFirst 1
          } else {
            continue
          }
        }
        set_ndr_layer_rule $tech $ndr [$layer getName] $value $isSpacing
        if { [$layer getName] == $lastLayer } {
          set foundLast 1
          break
        }
      }
      if { $foundFirst == 0 } {
        utl::warn ODB 1001 "Layer ${firstLayer} not found"
      }
      if { $foundLast == 0 } {
        utl::warn ODB 1002 "Layer ${lastLayer} not found"
      }
    }

  }
}

sta::define_cmd_args "create_ndr" { -name name \
                                  [-spacing val] \
                                  [-width val] \
                                  [-via val]}

proc create_ndr { args } {
  sta::parse_key_args "create_ndr" args keys {-name -spacing -width -via} flags {}
  if { ![info exists keys(-name)] } {
    utl::error ODB 1004 "-name is missing"
  }
  set name $keys(-name)
  set block [[[ord::get_db] getChip] getBlock]
  set tech [[ord::get_db] getTech]
  set ndr [odb::dbTechNonDefaultRule_create $block $name]
  if { $ndr == "NULL" } {
    utl::error ODB 1005 "NonDefaultRule ${name} already exists"
  }
  if { [info exists keys(-spacing)] } {
    set spacings $keys(-spacing)
    if { [llength $spacings] != 1 && [expr [llength $spacings] % 2] == 1 } {
      utl::error ODB 1006 "Spacing values \[$spacings\] are malformed"
    }
    set_ndr_rules $tech $ndr $spacings 1
  }
  if { [info exists keys(-width)] } {
    set widths $keys(-width)
    if { [llength $widths] != 1 &&  [expr [llength $widths] % 2] == 1 } {
      utl::error ODB 1007 "Width values \[$widths\] are malformed"
    }
    set_ndr_rules $tech $ndr $widths 0
  }
  if { [info exists keys(-via)] } {
    foreach viaName $keys(-via) {
      set via [$tech findVia $viaName]
      if { $via == "NULL" } {
        utl::error ODB 1008 "Via ${viaName} not found, skipping NDR for this via"
        continue
      }
      $ndr addUseVia $via
    }
  }

}

sta::define_cmd_args "create_voltage_domain" {domain_name -area {llx lly urx ury}}

proc create_voltage_domain { args } {
  sta::parse_key_args "create_voltage_domain" args keys {-area} flags {}
  set domain_name [lindex $args 0]
  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error ODB 315 "-area is a list of 4 coordinates"
    }
    lassign $area llx lly urx ury
    sta::check_positive_float "-area" $llx
    sta::check_positive_float "-area" $lly
    sta::check_positive_float "-area" $urx
    sta::check_positive_float "-area" $ury
  } else {
    utl::error ODB 316 "please define area"
  }
  sta::check_argc_eq1 "create_voltage_domain" $args
  set domain_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 317 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set region [odb::dbRegion_create $block $domain_name]
  if { $region == "NULL" } {
    utl::error ODB 318 "duplicate region name"
  }
  set box [odb::dbBox_create $region \
		[ord::microns_to_dbu $llx] [ord::microns_to_dbu $lly] \
		[ord::microns_to_dbu $urx] [ord::microns_to_dbu $ury]]
  set group [odb::dbGroup_create $region $domain_name]
  if { $group == "NULL" } {
    utl::error ODB 319 "duplicate group name"
  }
  $group setType VOLTAGE_DOMAIN
}

sta::define_cmd_args "delete_physical_cluster" {cluster_name};# checker off

proc delete_physical_cluster { args } {
  sta::check_argc_eq1 "delete_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 320 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    utl::error ODB 321 "group does not exist"
  }
  if { [$group getType] == "VOLTAGE_DOMAIN" } {
    utl::error ODB 322 "group is not of physical cluster type"
  }
  odb::dbGroup_destroy $group
}

sta::define_cmd_args "delete_voltage_domain" {domain_name}

proc delete_voltage_domain { args } {
  sta::parse_key_args "delete_voltage_domain" args keys {} flags {}
  sta::check_argc_eq1 "delete_voltage_domain" $args
  set domain_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 323 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  if { $group == "NULL" } {
    utl::error ODB 324 "group does not exist"
  }
  if { [$group getType] == "PHYSICAL_CLUSTER" } {
    utl::error ODB 325 "group is not of voltage domain type"
  }
  odb::dbGroup_destroy $group
}

sta::define_cmd_args "assign_power_net" {-domain domain_name -net snet_name}

proc assign_power_net { args } {
  sta::parse_key_args "assign_power_net" args keys {-domain -net} flags {}
  if { [info exists keys(-domain)] } {
    set domain_name $keys(-domain)
  } else {
    utl::error ODB 326 "define domain name"
  }
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  } else {
    utl::error ODB 327 "define net name"
  }
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 328 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  set net [$block findNet $net_name]
  if { $group == "NULL" } {
    utl::error ODB 329 "group does not exist"
  }
  if { [$group getType] == "PHYSICAL_CLUSTER" } {
    utl::error ODB 330 "group is not of voltage domain type"
  }
  if { $net == "NULL" } {
    utl::error ODB 331 "net does not exist"
  }
  $group addPowerNet $net
}

sta::define_cmd_args "assign_ground_net" {-domain domain_name -net snet_name}

proc assign_ground_net { args } {
  sta::parse_key_args "assign_ground_net" args keys {-domain -net} flags {}
  if { [info exists keys(-domain)] } {
    set domain_name $keys(-domain)
  } else {
    utl::error ODB 332 "define domain name"
  }
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  } else {
    utl::error ODB 333 "define net name"
  }
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 334 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  set net [$block findNet $net_name]
  if { $group == "NULL" } {
    utl::error ODB 335 "group does not exist"
  }
  if { [$group getType] == "PHYSICAL_CLUSTER" } {
    utl::error ODB 336 "group is not of voltage domain type"
  }
  if { $net == "NULL" } {
    utl::error ODB 337 "net does not exist"
  }
  $group addGroundNet $net
}

sta::define_cmd_args "add_to_physical_cluster" { [-modinst path | -inst inst_name | -physical_cluster cluster_name]  cluster_name }

proc add_to_physical_cluster { args } {
  sta::parse_key_args "add_to_physical_cluster" args keys {-modinst -inst -physical_cluster} flags {}
  sta::check_argc_eq1 "add_to_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 338 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    utl::error ODB 339 "cluster does not exist"
  }
  if { [$group getType] == "VOLTAGE_DOMAIN" } {
    utl::error ODB 340 "group is not of physical cluster type"
  }
  if { [info exists keys(-modinst)] } {
    set modinst [$block findModInst $keys(-modinst)]
    if { $modinst == "NULL" } {
      utl::error ODB 341 "modinst does not exist"
    }
    $group addModInst $modinst
  }
  if { [info exists keys(-inst)] } {
    set inst [$block findInst $keys(-inst)]
    if { $inst == "NULL" } {
      utl::error ODB 342 "inst does not exist"
    }
    $group addInst $inst
  }
  if { [info exists keys(-physical_cluster)] } {
    set child [$block findGroup $keys(-physical_cluster)]
    if { $child == "NULL" } {
      utl::error ODB 343 "child physical cluster does not exist"
    }
    if { [$child getType] == "VOLTAGE DOMAIN" } {
      utl::error ODB 344 "child group is not of physical cluster type"
    }
    $group addGroup $child
  }
}

sta::define_cmd_args "remove_from_physical_cluster" { [-parent_module module_name -modinst modinst_name | -inst inst_name | -physical_cluster cluster_name]  cluster_name }

proc remove_from_physical_cluster { args } {
  sta::parse_key_args "remove_from_physical_cluster" args keys {-parent_module -modinst -inst -physical_cluster} flags {}
  sta::check_argc_eq1 "remove_from_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 345 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    utl::error ODB 346 "cluster does not exist"
  }
  if { [$group getType] == "VOLTAGE_DOMAIN" } {
    utl::error ODB 347 "group is not of physical cluster type"
  }
  if { [info exists keys(-parent_module)] } {
    set module [$block findModule $keys(-parent_module)]
    if { $module == "NULL" } {
      utl::error ODB 348 "parent module does not exist"
    }
    set modinst [$module findModInst $keys(-modinst)]
    if { $modinst == "NULL" } {
      utl::error ODB 349 "modinst does not exist"
    }
    $group removeModInst $modinst
  }
  if { [info exists keys(-inst)] } {
    set inst [$block findInst $keys(-inst)]
    if { $inst == "NULL" } {
      utl::error ODB 350 "inst does not exist"
    }
    $group removeInst $inst
  }
  if { [info exists keys(-physical_cluster)] } {
    set child [$block findGroup $keys(-physical_cluster)]
    if { $child == "NULL" } {
      utl::error ODB 351 "child physical cluster does not exist"
    }
    if { [$child getType] == "VOLTAGE_DOMAIN" } {
      utl::error ODB 352 "child group is not of physical cluster type"
    }
    $group removeGroup $child
  }
}

sta::define_cmd_args "report_physical_clusters" {}

proc report_physical_clusters { args } {
  sta::parse_key_args "report_physical_clusters" args keys {} flags {}
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 353 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set groups [$block getGroups]
  utl::report "\nReporting Physical Clusters"
  foreach group $groups {
    if { [$group getType] == "PHYSICAL_CLUSTER" } {
      report_group $group
    }
  }
}

sta::define_cmd_args "report_voltage_domains" {}

proc report_voltage_domains { args } {
  sta::parse_key_args "report_voltage_domains" args keys {} flags {}
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 354 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set groups [$block getGroups]
  utl::report "\nReporting Voltage Domains"
  foreach group $groups {
    if { [$group getType] == "VOLTAGE_DOMAIN" } {
      report_group $group
    }
  }
}

sta::define_cmd_args "report_group" {group};# checker off
proc report_group { group } {
  utl::report "[expr \"[$group getType]\" == \"PHYSICAL_CLUSTER\" ? \"Physical Cluster\": \"Voltage Domain\"]: [$group getName]"
  if { [$group hasBox] } {
    set rect [$group getBox]
    utl::report "  * Box : ([$rect xMin],[$rect yMin]) ([$rect xMax],[$rect yMax])"
  }
  set modinsts [$group getModInsts]
  set insts [$group getInsts]
  set children [$group getGroups]
  set powerNets [$group getPowerNets]
  set groundNets [$group getGroundNets]
  if { [llength $modinsts] > 0 } {
    utl::report "  * ModInsts: "
    foreach modinst $modinsts {
      utl::report "    * [$modinst getHierarchalName]"
    }
  }
  if { [llength $insts] > 0 } {
    utl::report "  * Insts: "
    foreach inst $insts {
      utl::report "    * [$inst getName]"
    }
  }
  if { [llength $children] > 0 } {
    utl::report "  * Children: "
    foreach child $children {
      utl::report "    * [$child getName]"
    }
  }
  if { [llength $powerNets] > 0 } {
    utl::report "  * Power Nets: "
    foreach net $powerNets {
      utl::report "    * [$net getName]"
    }
  }
  if { [llength $groundNets] > 0 } {
    utl::report "  * Ground Nets: "
    foreach net $groundNets {
      utl::report "    * [$net getName]"
    }
  }
}

sta::define_cmd_args "write_guides" { filename }

proc write_guides { args } {
  sta::parse_key_args "write_guides" args keys {} flags {}
  sta::check_argc_eq1 "write_guides" $args
  set filename $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 355 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  $block writeGuides $filename
}

sta::define_cmd_args "write_macro_placement" { file_name }

proc write_macro_placement { args } {
  sta::parse_key_args "write_macro_placement" args keys {} flags {}
  sta::check_argc_eq1  "write_macro_placement" $args
  set file_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 439 "No design loaded. Cannot write macro placement."
  }
  set block [$chip getBlock]
  set macro_placement [odb::generateMacroPlacementString $block]

  set file [open $file_name w]
  puts $file $macro_placement
  close $file
}

sta::define_cmd_args "define_layer_range" { layers };# checker off

proc define_layer_range { layers } {
  set layer_range [grt::parse_layer_range "-layers" $layers]
  lassign $layer_range min_layer max_layer
  grt::check_routing_layer $min_layer
  grt::check_routing_layer $max_layer

  set_min_layer $min_layer
  set_max_layer $max_layer

  set tech [ord::get_db_tech]
  for {set layer 1} {$layer <= $max_layer} {set layer [expr $layer+1]} {
    set db_layer [$tech findRoutingLayer $layer]
    if { !([ord::db_layer_has_hor_tracks $db_layer] && \
         [ord::db_layer_has_ver_tracks $db_layer]) } {
      set layer_name [$db_layer getName]
      utl::error GRT 57 "Missing track structure for layer $layer_name."
    }
  }
}

sta::define_cmd_args "define_clock_layer_range" { layers };# checker off

proc define_clock_layer_range { layers } {
  set layer_range [grt::parse_layer_range "-clock_layers" $layers]
  lassign $layer_range min_clock_layer max_clock_layer
  grt::check_routing_layer $min_clock_layer
  grt::check_routing_layer $max_clock_layer

  if { $min_clock_layer < $max_clock_layer } {
    set db [ord::get_db]
    set chip [$db getChip]
    if { $chip == "NULL" } {
      utl::error ODB 363 "please load the design before trying to use this command"
    }
    set block [$chip getBlock]
    
    $block setMinLayerForClock $min_clock_layer
    $block setMaxLayerForClock $max_clock_layer
  } else {
    utl::error GRT 56 "In argument -clock_layers, min routing layer is\
      greater than max routing layer."
  }
}

sta::define_cmd_args "set_routing_layers" { [-signal min-max] \
                                            [-clock min-max] \
};# checker off
proc set_routing_layers { args } {
  sta::parse_key_args "set_routing_layers" args \
    keys {-signal -clock} flags {};# checker off

  sta::check_argc_eq0 "set_routing_layers" $args

  if { [info exists keys(-signal)] } {
    define_layer_range $keys(-signal)
  }

  if { [info exists keys(-clock)] } {
    define_clock_layer_range $keys(-clock)
  }
}

sta::define_cmd_args "set_min_layer" { minLayer };# checker off

proc set_min_layer { args } {
  sta::parse_key_args "set_min_layer" args keys {} flags {}
  sta::check_argc_eq1 "set_min_layer" $args
  set minLayer $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 365 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  $block setMinRoutingLayer $minLayer
}

sta::define_cmd_args "set_max_layer" { maxLayer };# checker off

proc set_max_layer { args } {
  sta::parse_key_args "set_max_layer" args keys {} flags {}
  sta::check_argc_eq1 "set_max_layer" $args
  set maxLayer $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    utl::error ODB 366 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  $block setMaxRoutingLayer $maxLayer
}
