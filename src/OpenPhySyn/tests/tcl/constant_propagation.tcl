import_lib ../tests/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
import_lef ../tests/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
import_def ../tests/data/designs/constants/constants.def
# import_def ../tests/data/designs/constants/constants_simple_no_opt.def
# import_def ../tests/data/designs/constants/constants_simple_no_opt.def
set propg [transform constant_propagation]
puts "Propagated $propg"
export_def ./outputs/propg.def
export_db ./outputs/prop.db

exit 0
