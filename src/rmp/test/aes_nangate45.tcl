# For debugging optimizations

read_liberty src/rmp/test/Nangate45/Nangate45_typ.lib
read_lef src/rmp/test/Nangate45/Nangate45.lef
read_lef src/rmp/test/Nangate45/Nangate45_stdcell.lef
read_verilog src/rmp/test/aes_nangate45.v
link_design aes_cipher_top
read_sdc src/rmp/test/aes_nangate45.sdc