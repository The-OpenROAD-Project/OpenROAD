
sta::define_cmd_args "create_physical_cluster" {cluster_name}

proc create_physical_cluster { args } {
  sta::check_argc_eq1 "create_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [odb::dbGroup_create $block $cluster_name]
  if { $group == "NULL" } {
    ord::error "duplicate group name"
  }
}

sta::define_cmd_args "create_child_physical_clusters" {[-top_module | -modinst path]}

proc create_child_physical_clusters { args } {
  sta::parse_key_args "create_child_physical_clusters" args keys {-modinst} flags {-top_module}
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  if { [info exists flags(-top_module)] } {
    set module [$block getTopModule]
  } elseif { [info exists keys(-modinst)] } {
    set module [[$block findModInst $keys(-modinst)] getMaster]
  } else {
    ord::error "please define either top module or the modinst path"
  }
  if { $module == "NULL" } {
    ord::error "module does not exist"
  }
  set module_instance [$module getModInst]
  set modinsts [$module getChildren]
  set insts [$module getInsts]
  foreach modinst $modinsts {
    set cluster_name "[$module getName]_[$modinst getName]"
    set group [odb::dbGroup_create $block $cluster_name]
    if { $group == "NULL" } {
      ord::error "duplicate group name"
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
      ord::error "duplicate group name"
    }
    foreach inst $insts {
      $group addInst $inst
    }
  }
}
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

sta::define_cmd_args "create_ndr" { -name name [-spacing val] [-width val] [-via val]}

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
      ord::error "-area is a list of 4 coordinates"
    }
    lassign $area llx lly urx ury
    sta::check_positive_float "-area" $llx
    sta::check_positive_float "-area" $lly
    sta::check_positive_float "-area" $urx
    sta::check_positive_float "-area" $ury
  } else {
    ord::error "please define area"
  }
  sta::check_argc_eq1 "create_voltage_domain" $args
  set domain_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [odb::dbGroup_create $block $domain_name \
		[ord::microns_to_dbu $llx] [ord::microns_to_dbu $lly] \
		[ord::microns_to_dbu $urx] [ord::microns_to_dbu $ury]]
  if { $group == "NULL" } {
    ord::error "duplicate group name"
  }
}

sta::define_cmd_args "delete_physical_cluster" {cluster_name}

proc delete_physical_cluster { args } {
  sta::check_argc_eq1 "delete_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == "VOLTAGE_DOMAIN" } {
    ord::error "group is not of physical cluster type"
  }
  odb::dbGroup_destroy $group
}

sta::define_cmd_args "delete_voltage_domain" {domain_name}

proc delete_voltage_domain { args } {
  sta::check_argc_eq1 "delete_voltage_domain" $args
  set domain_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == "PHYSICAL_CLUSTER" } {
    ord::error "group is not of voltage domain type"
  }
  odb::dbGroup_destroy $group
}

sta::define_cmd_args "assign_power_net" {-domain domain_name -net snet_name}

proc assign_power_net { args } {
  sta::parse_key_args "assign_power_net" args keys {-domain -net} flags {}
  if { [info exists keys(-domain)] } {
    set domain_name $keys(-domain)
  } else {
    ord::error "define domain name"
  }
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  } else {
    ord::error "define net name"
  }
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  set net [$block findNet $net_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == "PHYSICAL_CLUSTER" } {
    ord::error "group is not of voltage domain type"
  }
  if { $net == "NULL" } {
    ord::error "net does not exist"
  }
  $group addPowerNet $net
}

sta::define_cmd_args "assign_ground_net" {-domain domain_name -net snet_name}

proc assign_ground_net { args } {
  sta::parse_key_args "assign_ground_net" args keys {-domain -net} flags {}
  if { [info exists keys(-domain)] } {
    set domain_name $keys(-domain)
  } else {
    ord::error "define domain name"
  }
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  } else {
    ord::error "define net name"
  }
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  set net [$block findNet $net_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == "PHYSICAL_CLUSTER" } {
    ord::error "group is not of voltage domain type"
  }
  if { $net == "NULL" } {
    ord::error "net does not exist"
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
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    ord::error "cluster does not exist"
  }
  if { [$group getType] == "VOLTAGE_DOMAIN" } {
    ord::error "group is not of physical cluster type"
  }
  if { [info exists keys(-modinst)] } {
    set modinst [$block findModInst $keys(-modinst)]
    if { $modinst == "NULL" } {
      ord::error "modinst does not exist"
    }
    $group addModInst $modinst
  }
  if { [info exists keys(-inst)] } {
    set inst [$block findInst $keys(-inst)]
    if { $inst == "NULL" } {
      ord::error "inst does not exist"
    }
    $group addInst $inst
  }
  if { [info exists keys(-physical_cluster)] } {
    set child [$block findGroup $keys(-physical_cluster)]
    if { $child == "NULL" } {
      ord::error "child physical cluster does not exist"
    }
    if { [$child getType] == "VOLTAGE DOMAIN" } {
      ord::error "child group is not of physical cluster type"
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
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    ord::error "cluster does not exist"
  }
  if { [$group getType] == "VOLTAGE_DOMAIN" } {
    ord::error "group is not of physical cluster type"
  }
  if { [info exists keys(-parent_module)] } {
    set module [$block findModule $keys(-parent_module)]
    if { $module == "NULL" } {
      ord::error "parent module does not exist"
    }
    set modinst [$module findModInst $keys(-modinst)]
    if { $modinst == "NULL" } {
      ord::error "modinst does not exist"
    }
    $group removeModInst $modinst
  }
  if { [info exists keys(-inst)] } {
    set inst [$block findInst $keys(-inst)]
    if { $inst == "NULL" } {
      ord::error "inst does not exist"
    }
    $group removeInst $inst
  }
  if { [info exists keys(-physical_cluster)] } {
    set child [$block findGroup $keys(-physical_cluster)]
    if { $child == "NULL" } {
      ord::error "child physical cluster does not exist"
    }
    if { [$child getType] == "VOLTAGE_DOMAIN" } {
      ord::error "child group is not of physical cluster type"
    }
    $group removeGroup $child
  }
}

sta::define_cmd_args "report_physical_clusters" {}

proc report_physical_clusters {} {
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
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

proc report_voltage_domains {} {
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
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

# pre-logger compatibility for this file only
namespace eval ord {

proc error { args } {
 ord::error ODB 0 [lindex $args 0]
}

}
