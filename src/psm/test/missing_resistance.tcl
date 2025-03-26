source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

[[ord::get_db_tech] findLayer metal1] setResistance 0
[[ord::get_db_tech] findLayer via1] setResistance 0

catch {[analyze_power_grid -net VDD]} err
puts $err
