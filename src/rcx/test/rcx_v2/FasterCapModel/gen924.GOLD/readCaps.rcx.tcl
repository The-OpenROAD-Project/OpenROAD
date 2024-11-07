init_rcx_model -corner_names "TYP MIN" -met_cnt 7
read_rcx_tables -corner TYP -file  /home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/src/rcx/test/rcx_v2/FasterCapModel/du_parse.GOLD/fasterCap_3v2.standard.20.0.0.UnderDiag3.GOLD.caps 
read_rcx_tables -corner MIN -file  /home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/src/rcx/test/rcx_v2/FasterCapModel/ou_parse.GOLD/fasterCap_3v2.standard.20.0.0.OverUnder3.GOLD.caps 
read_rcx_tables -corner TYP -file  /home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/src/rcx/test/rcx_v2/FasterCapModel/M1uM3_parse.GOLD/fasterCap_3v1.normalized.20.20.20.M1uM3.GOLD.caps 
read_rcx_tables -corner TYP -file  /home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/src/rcx/test/rcx_v2/FasterCapModel/1v2_parse.GOLD/1v2_fc.standard.20.20.20.ALL.caps
read_rcx_tables -corner TYP -file  /home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/src/rcx/test/rcx_v2/FasterCapModel/1v1.GOLD/resistance.TYP 
write_rcx_model -file sept24.rcx.model 
