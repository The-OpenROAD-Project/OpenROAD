set block [ord::get_db_block]
set BName [$block getName]
set tech [ord::get_db_tech]
set BBOX [$block getBBox]
set defUnits [expr [$block getDefUnits] * 1.0]

set ury [expr [$BBOX  yMax] / $defUnits]
set urx [expr [$BBOX  xMax] / $defUnits]
set lly [expr [$BBOX  yMin] / $defUnits]
set llx [expr [$BBOX  xMin] / $defUnits]
set width [expr [$BBOX getDX] / $defUnits]
set height [expr [$BBOX getDY] / $defUnits]

set data "VERSION 5.7 ;\n  NOWIREEXTENSIONATPIN ON ;\n  DIVIDERCHAR \"/\" ;\n  BUSBITCHARS \"\[\]\" ;\nMACRO $BName \
\n  CLASS BLOCK ; \n  FOREIGN $BName ; \n  ORIGIN $llx $lly ; \n  SIZE  $width BY $height ; "

foreach PIN [$block getBTerms] {
  set data "$data \n  PIN [$PIN getName] "
  set data "$data \n    DIRECTION [$PIN getIoType] ;"
  set data "$data \n    USE [$PIN getSigType] ;"
  foreach PORT [$PIN getBPins] {
    set data "$data \n	 PORT"
    foreach BOX [$PORT getBoxes] {  
      set data "$data \n	   LAYER  [[$BOX getTechLayer] getName] ; "
      set data "$data \n	     RECT [expr [$BOX xMin] / $defUnits]  [expr [$BOX yMin] / $defUnits] [expr [$BOX xMax] / $defUnits] [expr [$BOX yMax] / $defUnits] ;"
      set PINSHAPE [odb::newSetFromRect  [$BOX xMin]  [$BOX yMin] [$BOX xMax]  [$BOX yMax]] 
      lappend PINSHAPES([[$BOX getTechLayer] getName]) $PINSHAPE 
    }
    set data "$data \n	 END "
  }
  set data "$data \n  END [$PIN getName]"
}


if {[array names  ::env "MAX_ROUTING_LAYER"]==""} {
  set MAX_ROUTING_LAYER [llength [[ord::get_db_tech] getLayers]]
} else {
  set MAX_ROUTING_LAYER $::env(MAX_ROUTING_LAYER)
} 

######## Create obstructions: 
set BLayers ""
set data "$data  \n  OBS" 

foreach layer [[ord::get_db_tech] getLayers] {
  if {[$layer getRoutingLevel] == 0} {continue}
  if {[$layer getRoutingLevel] > $MAX_ROUTING_LAYER} {continue}
  set data "$data  \n	 LAYER [$layer getName] ;"
  set layer_name [$layer getName] 

  if {[array names PINSHAPES $layer_name] != ""} {
    set pinshapes [odb::orSets $PINSHAPES($layer_name)] 
    set obstractions [odb::bloatSet $pinshapes [$layer getSpacing]]
    set obs [odb::newSetFromRect [$BBOX  xMin] [$BBOX  yMin] [$BBOX xMax] [$BBOX  yMax]]
    foreach rect [odb::getRectangles [odb::subtractSet $obs $obstractions]] {
      set data "$data  \n	   RECT [expr [$rect xMin] / $defUnits] [expr [$rect yMin] / $defUnits] [expr [$rect xMax] / $defUnits]  [expr [$rect yMax] / $defUnits] ;"
    }
  } else {
    set data "$data  \n	   RECT $llx $lly $urx $ury ;" 
  }
  
}

set data "$data  \n  END" 
set data "$data \nEND $BName \nEND LIBRARY"
set outputLef "$::env(RESULTS_DIR)/${BName}.lef"
set fileId [open $outputLef "w"]
puts -nonewline $fileId $data

close $fileId 
  

