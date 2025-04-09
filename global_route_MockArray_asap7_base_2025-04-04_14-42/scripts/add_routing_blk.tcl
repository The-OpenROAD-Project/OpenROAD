###################################################
# Create Routing Blockages around Macros for GF12 #
# Created by Minsoo Kim (mik226@eng.ucsd.edu)     #
###################################################
set db [::ord::get_db]
set block [[$db getChip] getBlock]
set tech [$db getTech]

set layer_M2 [$tech findLayer M2]
set layer_M3 [$tech findLayer M3]
set layer_C4 [$tech findLayer C4]

set numTrack 5

set allInsts [$block getInsts]

set cnt 0

foreach inst $allInsts {
  set master [$inst getMaster]
  set name [$master getName]
  set loc_llx [lindex [$inst getLocation] 0]
  set loc_lly [lindex [$inst getLocation] 1]

  if {[string match "*gf12*" $name]||[string match "IN12LP*" $name]} {
    set w [$master getWidth]
    set h [$master getHeight]

    set llx_Mx [expr $loc_llx - (128*$numTrack)] 
    set lly_Mx [expr $loc_lly - (128*$numTrack)] 
    set urx_Mx [expr $loc_llx + $w + (128*$numTrack)] 
    set ury_Mx [expr $loc_lly + $h + (128*$numTrack)] 

    set llx_Cx $loc_llx 
    set lly_Cx [expr $loc_lly - (160*$numTrack)] 
    set urx_Cx [expr $loc_llx + $w] 
    set ury_Cx [expr $loc_lly + $h + (160*$numTrack)] 

    set obs_M2 [odb::dbObstruction_create $block $layer_M2 $llx_Mx $lly_Mx $urx_Mx $ury_Mx]
    set obs_M3 [odb::dbObstruction_create $block $layer_M3 $llx_Mx $lly_Mx $urx_Mx $ury_Mx]
    set obs_C4 [odb::dbObstruction_create $block $layer_C4 $llx_Cx $lly_Cx $urx_Cx $ury_Cx]

    incr cnt
  }
}

if {$cnt != 0} {
  puts "Created $cnt routing blockages over macros"
}
