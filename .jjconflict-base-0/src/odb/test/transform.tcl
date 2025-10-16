set xfm [odb::dbTransform]
$xfm setOrient "R90"
puts [$xfm getOrient]

puts "pass"
