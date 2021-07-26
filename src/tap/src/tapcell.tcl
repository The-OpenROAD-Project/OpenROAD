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
                                [-tap_prefix tap_prefix]\
                                [-endcap_master endcap_master]\
                                [-endcap_cpp endcap_cpp]\
                                [-endcap_prefix endcap_prefix]\
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
              -incnrcap_nwout_master -tbtie_cpp -tap_prefix -endcap_prefix} \
    flags {-no_cell_at_top_bottom -add_boundary_cell}

  if { [info exists keys(-tapcell_master)] } {
    set tapcell_master $keys(-tapcell_master)
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
    utl::warn TAP 15 "tbtie_cpp option is deprecated."
  }
  
  if {[info exists flags(-no_cell_at_top_bottom)]} {
    utl::warn TAP 16 "no_cell_at_top_bottom option is deprecated."
  }
  if {[info exists flags(-add_boundary_cell)]} {
    utl::warn TAP 17 "add_boundary_cell option is deprecated."
  }
  
  set tap_prefix $tap::default_tapcell_prefix
  if { [info exists keys(-tap_prefix)] } {
      set tap_prefix $keys(-tap_prefix)
  }
  
  set endcap_prefix $tap::default_endcap_prefix
  if { [info exists keys(-endcap_prefix)] } {
      set endcap_prefix $keys(-endcap_prefix)
  }
  
  set add_boundary_cell 0
  if {$tap_nwintie_master != "INVALID" &&
      $tap_nwin2_master != "INVALID" &&
      $tap_nwin3_master != "INVALID" &&
      $tap_nwouttie_master != "INVALID" &&
      $tap_nwout2_master != "INVALID" &&
      $tap_nwout3_master != "INVALID" &&
      $incnrcap_nwin_master != "INVALID" &&
      $incnrcap_nwout_master != "INVALID"
  } {
      set add_boundary_cell 1
  }
  
  set db [ord::get_db]

  set halo_y [ord::microns_to_dbu $halo_y]
  set halo_x [ord::microns_to_dbu $halo_x]

  set endcap_master "NULL"
  if { [info exists keys(-endcap_master)] } {
    set endcap_master_name $keys(-endcap_master)
    set endcap_master [$db findMaster $endcap_master_name]
    if { $endcap_master == "NULL" } {
      utl::error TAP 10 "Master $endcap_master_name not found."
    }
  }

  tap::clear $tap_prefix $endcap_prefix

  tap::run $endcap_master $halo_x $halo_y $cnrcap_nwin_master $cnrcap_nwout_master $endcap_prefix $add_boundary_cell $tap_nwintie_master $tap_nwin2_master $tap_nwin3_master $tap_nwouttie_master $tap_nwout2_master $tap_nwout3_master $incnrcap_nwin_master $incnrcap_nwout_master $tapcell_master $dist $tap_prefix

}

sta::define_cmd_args "tapcell_ripup" {[-tap_prefix tap_prefix]\
                                      [-endcap_prefix endcap_prefix]
}

# This will remove the tap cells and endcaps to tapcell can be rerun with new parameters
proc tapcell_ripup { args } {
  sta::parse_key_args "tapcell" args \
    keys {-tap_prefix -endcap_prefix} \
    flags {}

  set tap_prefix $tap::default_tapcell_prefix
  if { [info exists keys(-tap_prefix)] } {
    set tap_prefix $keys(-tap_prefix)
  }

  set endcap_prefix $tap::default_endcap_prefix
  if { [info exists keys(-endcap_prefix)] } {
    set endcap_prefix $keys(-endcap_prefix)
  }

  set taps_removed [tap::remove_cells $tap_prefix]
  utl::info TAP 100 "Tap cells removed: $taps_removed"
  set endcaps_removed [tap::remove_cells $endcap_prefix]
  utl::info TAP 101 "Endcaps removed: $endcaps_removed"

  # Reset global parameters 
  tap::reset
}

namespace eval tap {
variable phy_idx
variable filled_sites
variable default_tapcell_prefix "TAP_"
variable default_endcap_prefix "PHY_"



# namespace end
}
