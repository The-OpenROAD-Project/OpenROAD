set db [::ord::get_db]
set block [[$db getChip] getBlock]

set nets [$block getNets]

set total_hpwl 0

foreach net $nets {

  set xMin 1e30
  set yMin 1e30
  set xMax -1e30
  set yMax -1e30

  foreach iTerm [$net getITerms] {
    set inst [$iTerm getInst]
    set instBox [$inst getBBox]
    
    # puts "instBox: [$instBox xMin] [$instBox xMax] [$instBox yMin] [$instBox yMax]" 
    set xMin [ expr min($xMin, [$instBox xMin]) ] 
    set yMin [ expr min($yMin, [$instBox yMin]) ] 
    set xMax [ expr max($xMax, [$instBox xMax]) ] 
    set yMax [ expr max($yMax, [$instBox yMax]) ] 
  }
  # puts "[$net getConstName]:instBox $xMin $yMin $xMax $yMax"

  foreach bTerm [$net getBTerms] {
    foreach bPin [$bTerm getBPins] {
      set pinBox [$bPin getBox]

      # puts "bPinBox: [$bTerm getConstName] [$pinBox xMin] [$pinBox yMin] [$pinBox xMax] [$pinBox yMax]"

      set xMin [ expr min($xMin, [$pinBox xMin]) ] 
      set yMin [ expr min($yMin, [$pinBox yMin]) ] 
      set xMax [ expr max($xMax, [$pinBox xMax]) ] 
      set yMax [ expr max($yMax, [$pinBox yMax]) ] 
    } 
  }
  # puts "final: [$net getConstName] $xMin $yMin $xMax $yMax"

  if { $xMin != 1e30 && $yMin != 1e30 && $xMax != -1e30 && $yMax != -1e30 } {
    set total_hpwl [expr $total_hpwl + $xMax - $xMin + $yMax - $yMin]
  }
}
puts "final hpwl: $total_hpwl"
