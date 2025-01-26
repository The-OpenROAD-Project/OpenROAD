source helpers.tcl
set test_name clust02


read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef
read_lib ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib 

set db [ord::get_db]
set chip [odb::dbChip_create $db]
set block [odb::dbBlock_create $chip "top"]
$block setDefUnits 1000

set clk [odb::dbNet_create $block "clk"]
$clk setSigType CLOCK

set master [$db findMaster "DFFHQNx1_ASAP7_75t_L"]

proc RandomInteger {max} {
    return [expr {int(rand()*$max)}]
}

set width 100
set height 100

expr { srand(17) }
for {set i 0} {$i < 100} {incr i} {
  set x [RandomInteger [expr $width * 1000]]
  set y [RandomInteger [expr $height * 1000]]

  set inst [odb::dbInst_create $block $master "inst_${x}_${y}"]
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED

  set clk_iterm [$inst findITerm "CLK"]
  $clk_iterm connect $clk

  set d_net [odb::dbNet_create $block "net_d_${x}_${y}"]
  set d_iterm [$inst findITerm "D"]
  $d_iterm connect $d_net
}

ord::design_created

initialize_floorplan \
    -die_area [list 0 0 $width $height] \
    -core_area [list 0 0 $width $height] \
    -site asap7sc7p5t

cluster_flops -tray_weight 60.0 \
              -timing_weight 1.0 \
              -max_split_size 50 \
              -num_paths 0
