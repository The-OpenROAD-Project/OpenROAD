read_liberty /workspace/w6/flow/platforms/nangate45/lib/NangateOpenCellLibrary_typical.lib
read_liberty /workspace/w6/flow/platforms/nangate45/lib/fakeram45_512x64.lib
read_liberty /workspace/w6/flow/platforms/nangate45/lib/fakeram45_256x96.lib
read_liberty /workspace/w6/flow/platforms/nangate45/lib/fakeram45_32x64.lib
read_liberty /workspace/w6/flow/platforms/nangate45/lib/fakeram45_64x7.lib
read_liberty /workspace/w6/flow/platforms/nangate45/lib/fakeram45_64x15.lib
read_liberty /workspace/w6/flow/platforms/nangate45/lib/fakeram45_64x96.lib

#read_db /workspace/w6/flow/results/nangate45/gcd/base/6_final.odb
#read_sdc /workspace/w6/flow/results/nangate45/gcd/base/6_final.sdc
#read_spef /workspace/w6/flow/results/nangate45/gcd/base/6_final.spef

read_db /workspace/w6/flow/results/nangate45/bp_multi/base/6_final.odb
read_sdc /workspace/w6/flow/results/nangate45/bp_multi/base/6_final.sdc
#read_spef /workspace/w6/flow/results/nangate45/bp_multi/base/6_final.spef

set_debug_level WEB tile 1
web_server -dir /workspace/w6/tools/OpenROAD/src/web/src
