set db [ord::get_db]
set chip [odb::dbChip_create $db]
set tech [odb::dbTech_create $db tech]
set top_block [odb::dbBlock_create $chip top]

set child_block [odb::dbBlock_create $top_block child]
set child_net [odb::dbNet_create $child_block child_net]

set inst [odb::dbInst_create $top_block $child_block inst]

puts BEFORE
foreach bterm [$child_block getBTerms] { puts "bterm [$bterm getName]" }
foreach iterm [$inst getITerms] { puts "iterm [[$iterm getMTerm] getName]" }
foreach mterm [[$inst getMaster] getMTerms] { puts "mterm [$mterm getName]" }

odb::dbBTerm_create $child_net term

puts AFTER
foreach bterm [$child_block getBTerms] { puts "bterm [$bterm getName]" }
foreach iterm [$inst getITerms] { puts "iterm [[$iterm getMTerm] getName]" }
foreach mterm [[$inst getMaster] getMTerms] { puts "mterm [$mterm getName]" }
