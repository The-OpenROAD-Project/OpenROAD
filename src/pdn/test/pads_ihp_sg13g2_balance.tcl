# Reduced version of the design from
# https://github.com/The-OpenROAD-Project/OpenROAD/issues/9994:
# a sg13g2 pad ring around an empty core, with the core straps and the core
# ring sharing TopMetal1/TopMetal2. The core straps block the pad to core ring
# connections, and the connections that survive are heavily biased towards
# one net (VDD gets several per side, VSS only one).
source "helpers.tcl"

read_lef ihp_ethmac/tech.lef
read_lef ihp_ethmac/sg13g2.lef
read_lef ihp_tensorcore/sg13g2_io.lef

read_def ihp_tensorcore/floorplan.def

# reports, per net, which pads got connected to the core ring and how those
# connections are distributed over the four edges of the die
proc report_pad_connections { } {
  set block [ord::get_db_block]
  set core [$block getCoreArea]
  set cx [expr ([$core xMin] + [$core xMax]) / 2]
  set cy [expr ([$core yMin] + [$core yMax]) / 2]

  foreach net_name {VDD VSS} {
    set net [$block findNet $net_name]
    set shapes {}
    foreach swire [$net getSWires] {
      foreach sbox [$swire getWires] {
        if { [$sbox isVia] } {
          continue
        }
        lappend shapes [list [$sbox xMin] [$sbox yMin] [$sbox xMax] [$sbox yMax]]
      }
    }

    set counts [dict create north 0 south 0 east 0 west 0]
    set connected {}
    foreach inst [$block getInsts] {
      if { ![[$inst getMaster] isPad] } {
        continue
      }
      set bbox [$inst getBBox]
      set connects 0
      foreach shape $shapes {
        lassign $shape x_min y_min x_max y_max
        if {
          $x_min < [$bbox xMax] && $x_max > [$bbox xMin] &&
          $y_min < [$bbox yMax] && $y_max > [$bbox yMin]
        } {
          set connects 1
          break
        }
      }
      if { !$connects } {
        continue
      }

      set inst_x [expr ([$bbox xMin] + [$bbox xMax]) / 2]
      set inst_y [expr ([$bbox yMin] + [$bbox yMax]) / 2]
      if { abs($inst_x - $cx) > abs($inst_y - $cy) } {
        set edge [expr { $inst_x < $cx ? "west" : "east" }]
      } else {
        set edge [expr { $inst_y < $cy ? "south" : "north" }]
      }
      dict incr counts $edge
      lappend connected [$inst getName]
    }

    puts "$net_name: [llength $connected] pad connections\
(north [dict get $counts north],\
 south [dict get $counts south],\
 east [dict get $counts east],\
 west [dict get $counts west])"
    foreach inst_name [lsort $connected] {
      puts "  $inst_name"
    }
  }
}

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid \
  -name stdcell_grid \
  -starts_with POWER \
  -voltage_domains {CORE}

add_pdn_stripe \
  -grid stdcell_grid \
  -layer TopMetal1 \
  -width 2.2 \
  -pitch 75.6 \
  -offset 13.6 \
  -spacing 4 \
  -starts_with POWER \
  -extend_to_core_ring

add_pdn_stripe \
  -grid stdcell_grid \
  -layer TopMetal2 \
  -width 2.2 \
  -pitch 75.6 \
  -offset 13.6 \
  -spacing 4 \
  -starts_with POWER \
  -extend_to_core_ring

add_pdn_connect -grid stdcell_grid -layers "TopMetal1 TopMetal2"

add_pdn_ring \
  -grid stdcell_grid \
  -layers "TopMetal1 TopMetal2" \
  -widths "15 15" \
  -spacings "5 5" \
  -core_offsets "4.5 4.5" \
  -allow_out_of_die \
  -connect_to_pads

set_debug_level PDN Make 2

pdngen

report_pad_connections

set def_file [make_result_file pads_ihp_sg13g2_balance.def]
write_def $def_file
diff_files pads_ihp_sg13g2_balance.defok $def_file
