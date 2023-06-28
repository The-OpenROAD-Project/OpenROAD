# OpenROAD Messages Glossary
Listed below are the OpenROAD warning/error codes you may encounter while using the application.

| Tool | Code | Message                                             |
| ---- | ---- | --------------------------------------------------- |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.cc#L1773) | 0001 | Found {} pin violations. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.cc#L1771) | 0002 | Found {} net violations. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.cc#L1730) | 0008 | No detailed or global routing found. Run global_route or detailed_route first. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.cc#L1993) | 0009 | Net {} requires more than {} diodes per gate to repair violations. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.tcl#L57) | 0010 | -report_filename is deprecated. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.tcl#L60) | 0011 | -report_violating_nets is deprecated. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.i#L66) | 0012 | Net {} not found. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.cc#L205) | 0013 | No THICKNESS is provided for layer {}. Checks on this layer will not be correct. |
| [ANT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ant/src/AntennaChecker.cc#L1755) | 0014 | Skipped net {} because it is special. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L143) | 0001 | Running TritonCTS with user-specified clock roots: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L348) | 0003 | Total number of Clock Roots: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L352) | 0004 | Total number of Buffers Inserted: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L356) | 0005 | Total number of Clock Subnets: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L360) | 0006 | Total number of Sinks: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L465) | 0007 | Net \"{}\" found for clock \"{}\". |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L490) | 0008 | TritonCTS found {} clock nets. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L575) | 0010 | Clock net \"{}\" has {} sinks. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L721) | 0012 | Minimum number of buffers in the clock path: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L723) | 0013 | Maximum number of buffers in the clock path: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L727) | 0014 | {} clock nets were removed/fixed. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L731) | 0015 | Created {} clock nets. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L740) | 0016 | Fanout distribution for the current clock = {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L744) | 0017 | Max level of the clock tree: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L838) | 0018 | Created {} clock buffers. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L155) | 0019 | Total number of sinks after clustering: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L167) | 0020 | Wire segment unit: {} dbu ({} um). |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L176) | 0021 | Distance between buffers: {} units ({} um). |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L184) | 0022 | Branch length for Vertex Buffer: {} units ({} um). |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L204) | 0023 | Original sink region: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L221) | 0024 | Normalized sink region: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L222) | 0025 | Width: {:.4f}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L223) | 0026 | Height: {:.4f}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L228) | 0027 | Generating H-Tree topology for net {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L230) | 0028 | Total number of sinks: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L236) | 0029 | Sinks will be clustered in groups of up to {} and with maximum cluster diameter of {:.1f} um. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L244) | 0030 | Number of static layers: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L266) | 0031 | Stop criterion found. Min length of sink region is ({}). |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L278) | 0032 | Stop criterion found. Max number of sinks is {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L363) | 0034 | Segment length (rounded): {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L880) | 0035 | Number of sinks covered: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L1162) | 0038 | Number of created patterns = {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L1179) | 0039 | Number of created patterns = {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L477) | 0040 | Net was not found in the design for {}, please check. Skipping... |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L561) | 0041 | Net \"{}\" has {} sinks. Skipping... |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L570) | 0042 | Net \"{}\" has no sinks. Skipping... |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L135) | 0043 | {} wires are pure wire and no slew degradation.\nTritonCTS forced slew degradation on these wires. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L382) | 0045 | Creating fake entries in the LUT. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L142) | 0046 | Number of wire segments: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L144) | 0047 | Number of keys in characterization LUT: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L149) | 0048 | Actual min input cap: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L504) | 0049 | Characterization buffer is: {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.tcl#L151) | 0055 | Missing argument -buf_list |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.tcl#L163) | 0056 | Error when finding -clk_nets in DB. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.tcl#L184) | 0057 | Missing argument, user must enter at least one of -root_buf or -buf_list. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/SinkClustering.cpp#L153) | 0058 | Invalid parameters in {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L178) | 0065 | Normalized values in the LUT should be in the range [1, {}\n Check the table above to see the normalization ranges and your characterization configuration. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L456) | 0073 | Buffer not found. Check your -buf_list input. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L461) | 0074 | Buffer {} not found. Check your -buf_list input. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L566) | 0075 | Error generating the wirelengths to test.\n Check the -wire_unit parameter or the technology files. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L610) | 0076 | No Liberty cell found for {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L637) | 0078 | Error generating the wirelengths to test.\n Check the parameters -max_cap/-max_slew/-cap_inter/-slew_inter\n or the technology files. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L108) | 0079 | Sink not found. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L872) | 0080 | Sink not found. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L128) | 0081 | Buffer {} is not in the loaded DB. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L93) | 0082 | No valid clock nets in the design. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L487) | 0083 | No clock nets have been found. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L80) | 0084 | Compiling LUT. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L702) | 0085 | Could not find the root of {} |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L334) | 0087 | Could not open output metric file {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/HTreeBuilder.cpp#L233) | 0090 | Sinks will be clustered based on buffer max cap. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/LevelBalancer.cpp#L52) | 0093 | Fixing tree levels for max depth {} |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L463) | 0095 | Net \"{}\" found. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L587) | 0096 | No Liberty cell found for {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L134) | 0097 | Characterization used {} buffer(s) types. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L309) | 0098 | Clock net \"{}\" |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L310) | 0099 | Sinks {} |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L311) | 0100 | Leaf buffers {} |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L313) | 0101 | Average sink wire length {:.2f} um |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L314) | 0102 | Path depth {} - {} |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.tcl#L199) | 0103 | No design block found. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L436) | 0104 | Clock wire resistance/capacitance values are zero.\nUse set_wire_rc to set them. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L542) | 0105 | Net \"{}\" already has clock buffer {}. Skipping... |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L468) | 0106 | No Liberty found for buffer {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L597) | 0107 | No max slew found for cell {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L604) | 0108 | No max capacitance found for cell {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L488) | 0111 | No max capacitance found for cell {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L498) | 0113 | Characterization buffer is not defined.\n Check that -buf_list has supported buffers from platform. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.cpp#L447) | 0114 | Clock {} overlaps a previous clock. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TritonCTS.tcl#L99) | 0115 | -post_cts_disable is obsolete. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L522) | 0534 | Could not find buffer input port for {}. |
| [CTS](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/cts/src/TechChar.cpp#L527) | 0541 | Could not find buffer output port for {}. |
| [DFT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dft/src/replace/ScanReplace.cpp#L384) | 0002 | Can't scan replace cell '{:s}', that has lib cell '{:s}'. No scan equivalent lib cell found |
| [DFT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dft/src/replace/ScanReplace.cpp#L374) | 0003 | Cell '{:s}' is already an scan cell, we will not replace it |
| [DFT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dft/src/clock_domain/ClockDomain.cpp#L52) | 0004 | Clock mix config requested is not supported |
| [DFT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dft/src/cells/ScanCellFactory.cpp#L137) | 0005 | Cell '{:s}' doesn't have a valid clock connected. Can't create a scan cell |
| [DFT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dft/src/dft.i#L80) | 0006 | Requested clock mixing config not valid |
| [DFT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dft/src/cells/ScanCellFactory.cpp#L146) | 0007 | Cell '{:s}' is not a scan cell. Can't use it for scan architect |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/FillerPlacement.cpp#L73) | 0001 | Placed {} filler instances. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/FillerPlacement.cpp#L110) | 0002 | could not fill gap of size {} at {},{} dbu between {} and {} |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/dbToOpendp.cpp#L131) | 0012 | no rows found. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Grid.cpp#L612) | 0013 | Cannot paint grid because it is already occupied. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Place.cpp#L323) | 0015 | instance {} does not fit inside the ROW core area. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Place.cpp#L462) | 0016 | cannot place instance {}. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Place.cpp#L524) | 0017 | cannot place instance {}. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/OptMirror.cpp#L98) | 0020 | Mirrored {} instances |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/OptMirror.cpp#L100) | 0021 | HPWL before {:8.1f} u |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/OptMirror.cpp#L102) | 0022 | HPWL after {:8.1f} u |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/OptMirror.cpp#L107) | 0023 | HPWL delta {:8.1f} % |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Place.cpp#L1319) | 0026 | legalPt called on fixed cell. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.tcl#L73) | 0027 | no rows defined in design. Use initialize_floorplan to add rows. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.tcl#L203) | 0028 | $name did not match any masters. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.tcl#L220) | 0029 | cannot find instance $inst_name |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.tcl#L235) | 0030 | cannot find instance $inst_name |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.tcl#L53) | 0031 | -max_displacement disp|{disp_x disp_y} |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.tcl#L178) | 0032 | Debug instance $instance_name not found. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/CheckPlacement.cpp#L107) | 0033 | detailed placement checks failed. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.cpp#L169) | 0034 | Detailed placement failed on the following {} instances: |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.cpp#L174) | 0035 | {} |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.cpp#L176) | 0036 | Detailed placement failed. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.cpp#L141) | 0037 | Use remove_fillers before detailed placement. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.cpp#L157) | 0038 | No 1-site fill cells detected. To remove 1-site gaps use the -disallow_one_site_gaps flag. |
| [DPL](masters.) | 0039 | \"$arg\" did not match any |
| [This](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.tcl#L207) | could | due to a change from using regex to glob to search for cell masters. https://github.com/The-OpenROAD-Project/OpenROAD/pull/3210 |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Grid.cpp#L641) | 0041 | Cannot paint grid because another layer is already occupied. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Grid.cpp#L497) | 0042 | No cells found in group {}. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.cpp#L569) | 0043 | No grid layers mapped. |
| [DPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpl/src/Opendp.cpp#L574) | 0044 | Cell {} with height {} is taller than any row. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed.cxx#L174) | 0001 | Unknown algorithm {:s}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.tcl#L59) | 0031 | -max_displacement disp|{disp_x disp_y} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L431) | 0100 | Creating network with {:d} cells, {:d} terminals, {:d} edges and {:d} pins. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L560) | 0101 | Unexpected total node count. Expected {:d}, but got {:d} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L594) | 0102 | Improper node indexing while connecting pins. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L622) | 0103 | Could not find node for instance while connecting pins. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L635) | 0104 | Improper terminal indexing while connecting pins. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L651) | 0105 | Could not find node for terminal while connecting pins. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L661) | 0106 | Unexpected total edge count. Expected {:d}, but got {:d} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L668) | 0107 | Unexpected total pin count. Expected {:d}, but got {:d} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L750) | 0108 | Skipping all the rows with sites {} as their height is {} and the single-height is {}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L675) | 0109 | Network stats: inst {}, edges {}, pins {} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/Optdp.cpp#L929) | 0110 | Number of regions is {:d} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/legalize_shift.cxx#L198) | 0200 | Unexpected displacement during legalization. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/legalize_shift.cxx#L214) | 0201 | Placement check failure during legalization. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_mis.cxx#L185) | 0202 | No movable cells found |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L398) | 0203 | No movable cells found |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_mis.cxx#L164) | 0300 | Set matching objective is {:s}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_mis.cxx#L194) | 0301 | Pass {:3d} of matching; objective is {:.6e}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_mis.cxx#L226) | 0302 | End of matching; objective is {:.6e}, improvement is {:.2f} percent. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed.cxx#L176) | 0303 | Running algorithm for {:s}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_reorder.cxx#L113) | 0304 | Pass {:3d} of reordering; objective is {:.6e}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_reorder.cxx#L127) | 0305 | End of reordering; objective is {:.6e}, improvement is {:.2f} percent. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_global.cxx#L133) | 0306 | Pass {:3d} of global swaps; hpwl is {:.6e}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_global.cxx#L141) | 0307 | End of global swaps; objective is {:.6e}, improvement is {:.2f} percent. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_vertical.cxx#L135) | 0308 | Pass {:3d} of vertical swaps; hpwl is {:.6e}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_vertical.cxx#L147) | 0309 | End of vertical swaps; objective is {:.6e}, improvement is {:.2f} percent. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L949) | 0310 | Assigned {:d} cells into segments. Movement in X-direction is {:f}, movement in Y-direction is {:f}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1351) | 0311 | Found {:d} overlaps between adjacent cells. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1406) | 0312 | Found {:d} edge spacing violations and {:d} padding violations. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1587) | 0313 | Found {:d} cells in wrong regions. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1645) | 0314 | Found {:d} site alignment problems. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1677) | 0315 | Found {:d} row alignment problems. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_abu.cxx#L422) | 0317 | ABU: Target {:.2f}, ABU_2,5,10,20: {:.2f}, {:.2f}, {:.2f}, {:.2f}, Penalty {:.2f} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1187) | 0318 | Collected {:d} single height cells. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1231) | 0319 | Collected {:d} multi-height cells spanning {:d} rows. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1261) | 0320 | Collected {:d} fixed cells (excluded terminal_NI). |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1293) | 0321 | Collected {:d} wide cells. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L303) | 0322 | Image ({:d}, {:d}) - ({:d}, {:d}) |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed.cxx#L131) | 0323 | One site gap violation in segment {:d} nodes: {} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L192) | 0324 | Random improver is using {:s} generator. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L242) | 0325 | Random improver is using {:s} objective. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L264) | 0326 | Random improver cost string is {:s}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L302) | 0327 | Pass {:3d} of random improver; improvement in cost is {:.2f} percent. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L321) | 0328 | End of random improver; improvement is {:.6f} percent. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L390) | 0329 | Random improver requires at least one generator. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L428) | 0330 | Test objective function failed, possibly due to a badly formed cost function. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L501) | 0332 | End of pass, Generator {:s} called {:d} times. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L515) | 0333 | End of pass, Objective {:s}, Initial cost {:.6e}, Scratch cost {:.6e}, Incremental cost {:.6e}, Mismatch? {:c} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_global.cxx#L542) | 0334 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L671) | 0335 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_vertical.cxx#L535) | 0336 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L843) | 0337 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L534) | 0338 | End of pass, Total cost is {:.6e}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1467) | 0339 | One-site gap violation detected with a Fixed/Terminal cell {} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L1477) | 0340 | One-site gap violation detected with a multi-height cell {} |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_orient.cxx#L104) | 0380 | Cell flipping. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_orient.cxx#L112) | 0381 | Encountered {:d} issues when orienting cells for rows. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_orient.cxx#L118) | 0382 | Changed {:d} cell orientations for row compatibility. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_orient.cxx#L127) | 0383 | Performed {:d} cell flips. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_orient.cxx#L132) | 0384 | End of flipping; objective is {:.6e}, improvement is {:.2f} percent. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_random.cxx#L589) | 0385 | Only working with single height cells currently. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L171) | 0400 | Detailed improvement internal error: {:s}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L134) | 0401 | Setting random seed to {:d}. |
| [DPO](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dpo/src/detailed_manager.cxx#L152) | 0402 | Setting maximum displacement {:d} {:d} to {:d} {:d} units. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_init.cpp#L334) | 0000 | initNetTerms unsupported obj. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L200) | 0002 | Detailed routing has not been run yet. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3316) | 0003 | Load design first. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3322) | 0004 | Load design first. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L118) | 0005 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L129) | 0006 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L147) | 0007 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L315) | 0008 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L341) | 0009 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L351) | 0010 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L375) | 0011 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L386) | 0012 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L397) | 0013 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L445) | 0014 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L462) | 0015 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L507) | 0016 | Unsupported region query add of blockage in instance {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L526) | 0017 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L729) | 0018 | Complete {} insts. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L733) | 0019 | Complete {} insts. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L745) | 0020 | Complete {} terms. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L749) | 0021 | Complete {} terms. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L766) | 0022 | Complete {} snets. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L777) | 0023 | Complete {} blockages. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L787) | 0024 | Complete {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L817) | 0026 | Complete {} origin guides. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L821) | 0027 | Complete {} origin guides. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L832) | 0028 | Complete {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L861) | 0029 | Complete {} nets (guide). |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L865) | 0030 | Complete {} nets (guide). |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L158) | 0031 | Unsupported region query add. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L993) | 0032 | {} grObj region query size = {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L1007) | 0033 | {} shape region query size = {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L1035) | 0034 | {} drObj region query size = {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L875) | 0035 | Complete {} (guide). |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L1021) | 0036 | {} guide region query size = {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_init.cpp#L81) | 0037 | init_design_helper shape does not have net. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_init.cpp#L92) | 0038 | init_design_helper shape does not have dr net. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_init.cpp#L98) | 0039 | init_design_helper unsupported type. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L150) | 0041 | Unsupported metSpc rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L225) | 0042 | Unknown corner direction. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L300) | 0043 | Unsupported metSpc rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2326) | 0044 | Unsupported LEF58_SPACING rule for cut layer, skipped. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2503) | 0045 | Unsupported branch EXACTALIGNED in checkLef58CutSpacing_spc_adjCut. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2510) | 0046 | Unsupported branch EXCEPTSAMEPGNET in checkLef58CutSpacing_spc_adjCut. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2517) | 0047 | Unsupported branch EXCEPTALLWITHIN in checkLef58CutSpacing_spc_adjCut. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2524) | 0048 | Unsupported branch TO ALL in checkLef58CutSpacing_spc_adjCut. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2531) | 0050 | Unsupported branch ENCLOSURE in checkLef58CutSpacing_spc_adjCut. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2538) | 0051 | Unsupported branch SIDEPARALLELOVERLAP in checkLef58CutSpacing_spc_adjCut. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2545) | 0052 | Unsupported branch SAMEMASK in checkLef58CutSpacing_spc_adjCut. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_init.cpp#L986) | 0053 | updateGCWorker cannot find frNet in DRWorker. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2636) | 0054 | Unsupported branch STACK in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2640) | 0055 | Unsupported branch ORTHOGONALSPACING in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2648) | 0056 | Unsupported branch SHORTEDGEONLY in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2655) | 0057 | Unsupported branch WIDTH in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2660) | 0058 | Unsupported branch PARALLEL in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2665) | 0059 | Unsupported branch EDGELENGTH in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2671) | 0060 | Unsupported branch EXTENSION in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2678) | 0061 | Unsupported branch ABOVEWIDTH in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2683) | 0062 | Unsupported branch MASKOVERLAP in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L2688) | 0063 | Unsupported branch WRONGDIRECTION in checkLef58CutSpacing_spc_layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_init.cpp#L92) | 0065 | instAnalysis unsupported pinFig. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_init.cpp#L99) | 0066 | instAnalysis skips {} due to no pin shapes. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L118) | 0067 | FlexPA mergePinShapes unsupported shape. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L457) | 0068 | prepPoint_pin_genPoints_rect cannot find secondLayerNum. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_init.cpp#L261) | 0069 | initPinAccess error. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L801) | 0070 | Unexpected direction in getPlanarEP. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1303) | 0071 | prepPoint_pin_helper unique2paidx not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1316) | 0072 | prepPoint_pin_helper unique2paidx not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1467) | 0073 | No access point for {}/{}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1516) | 0074 | No access point for PIN/{}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1405) | 0075 | prepPoint_pin unique2paidx not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1479) | 0076 | Complete {} pins. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1483) | 0077 | Complete {} pins. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1527) | 0078 | Complete {} pins. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1689) | 0079 | Complete {} unique inst patterns. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1694) | 0080 | Complete {} unique inst patterns. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1706) | 0081 | Complete {} unique inst patterns. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1620) | 0082 | Complete {} groups. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1624) | 0083 | Complete {} groups. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1636) | 0084 | Complete {} groups. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1940) | 0085 | Valid access pattern combination not found for {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L2202) | 0086 | Pin does not have an access point. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1675) | 0087 | No valid pattern for unique instance {}, master is {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L2385) | 0089 | genPattern_gc objs empty. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L2634) | 0090 | Valid access pattern not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L2707) | 0091 | Pin does not have valid ap. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/rp/FlexRP_prep.cpp#L1468) | 0092 | Duplicate diff layer samenet cut spacing, skipping cut spacing from {} to {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/rp/FlexRP_prep.cpp#L1492) | 0093 | Duplicate diff layer diffnet cut spacing, skipping cut spacing from {} to {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L91) | 0094 | Cannot find layer: {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L133) | 0095 | Library cell {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L137) | 0096 | Same cell name: {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L256) | 0097 | Cannot find cut layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L266) | 0098 | Cannot find bottom layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L277) | 0099 | Cannot find top layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L361) | 0100 | Unsupported via: {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L363) | 0101 | Non-consecutive layers for via: {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L502) | 0102 | Odd dimension in both directions. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L531) | 0103 | Unknown direction. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L576) | 0104 | Terminal {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L601) | 0105 | Component {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L607) | 0106 | Component pin {}/{} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L686) | 0107 | Unsupported layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L811) | 0108 | Unsupported via in db. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L874) | 0109 | Unsupported via in db. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1577) | 0110 | Complete {} groups. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1581) | 0111 | Complete {} groups. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L999) | 0112 | Unsupported layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3010) | 0113 | Tech layers for via {} not found in db tech. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3142) | 0114 | Unknown connFig type while writing net {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L66) | 0115 | Setting MAX_THREADS=1 for use with the PA GUI. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1156) | 0116 | Load design first. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1159) | 0117 | Load design first. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L266) | 0118 | -worker is a list of 2 coordinates. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L270) | 0119 | Marker ({}, {}) ({}, {}) on {}: |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2242) | 0122 | Layer {} is skipped for {}/{}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2277) | 0123 | Layer {} is skipped for {}/OBS. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2470) | 0124 | Via {} with unused layer {} will be ignored. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2485) | 0125 | Unsupported via {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2492) | 0126 | Non-consecutive layers for via {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2502) | 0127 | Unknown layer {} for via {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2353) | 0128 | Unsupported viarule {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2359) | 0129 | Unknown layer {} for viarule {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2372) | 0130 | Non-consecutive layers for viarule {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2393) | 0131 | cutLayer cannot have overhangs in viarule {}, skipping enclosure. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2414) | 0132 | botLayer cannot have rect in viarule {}, skipping rect. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2424) | 0133 | topLayer cannot have rect in viarule {}, skipping rect. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2439) | 0134 | botLayer cannot have spacing in viarule {}, skipping spacing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2449) | 0135 | botLayer cannot have spacing in viarule {}, skipping spacing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2552) | 0136 | Load design first. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1896) | 0138 | New SPACING SAMENET overrides oldSPACING SAMENET rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1838) | 0139 | minEnclosedArea constraint with width is not supported, skipped. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1868) | 0140 | SpacingRange unsupported. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1870) | 0141 | SpacingLengthThreshold unsupported. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1872) | 0142 | SpacingNotchLength unsupported. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1874) | 0143 | SpacingEndOfNotchWidth unsupported. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1912) | 0144 | New SPACING SAMENET overrides oldSPACING SAMENET rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1966) | 0145 | New SPACINGTABLE PARALLELRUNLENGTH overrides old SPACING rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1995) | 0146 | New SPACINGTABLE TWOWIDTHS overrides old SPACING rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2117) | 0147 | cutWithin is smaller than cutSpacing for ADJACENTCUTS on layer {}, please check your rule definition. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1130) | 0148 | Deprecated lef param in params file. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2547) | 0149 | Reading tech and libs. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1151) | 0150 | Reading design. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2610) | 0153 | Cannot find net {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2614) | 0154 | Cannot find layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2631) | 0155 | Guide in net {} uses layer {} ({}) that is outside the allowed routing range [{} ({}), ({})]. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2652) | 0156 | guideIn read {} guides. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2656) | 0157 | guideIn read {} guides. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L122) | 0160 | Warning: {} does not have viaDef aligned with layer direction, generating new viaDef {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1582) | 0161 | Unsupported LEF58_SPACING rule for layer {} of type MAXXY. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_pin.cpp#L37) | 0162 | Library cell analysis. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_pin.cpp#L83) | 0163 | Instance analysis. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_pin.cpp#L137) | 0164 | Number of unique instances = {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA.cpp#L163) | 0165 | Start pin access. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA.cpp#L201) | 0166 | Complete pin access. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/db/tech/frTechObject.h#L257) | 0167 | List of default vias: |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L566) | 0168 | Init region query. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L578) | 0169 | Post process guides. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L763) | 0170 | No GCELLGRIDX. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L768) | 0171 | No GCELLGRIDY. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L822) | 0172 | No GCELLGRIDX. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L827) | 0173 | No GCELLGRIDY. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L867) | 0174 | GCell cnt x < 1. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L883) | 0175 | GCell cnt y < 1. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L889) | 0176 | GCELLGRID X {} DO {} STEP {} ; |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L895) | 0177 | GCELLGRID Y {} DO {} STEP {} ; |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L616) | 0178 | Init guide query. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L619) | 0179 | Init gr pin query. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2986) | 0180 | Post processing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA.cpp#L334) | 0181 | Start track assignment. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA.cpp#L347) | 0182 | Complete track assignment. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA.cpp#L224) | 0183 | Done with {} horizontal wires in {} frboxes and {} vertical wires in {} frboxes. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA.cpp#L242) | 0184 | Done with {} vertical wires in {} frboxes and {} horizontal wires in {} frboxes. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L635) | 0185 | Post process initialize RPin region query. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA.cpp#L308) | 0186 | Done with {} vertical wires in {} frboxes and {} horizontal wires in {} frboxes. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L393) | 0187 | Start routing data preparation. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L407) | 0194 | Start detail routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L486) | 0195 | Start {}{} optimization iteration. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L760) | 0198 | Complete detail routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L702) | 0199 | Number of violations = {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gr/FlexGR.cpp#L626) | 0201 | Must load design before global routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gr/FlexGR.cpp#L654) | 0202 | Skipping layer {} not found in db for congestion map. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gr/FlexGR.cpp#L633) | 0203 | dbGcellGrid already exists in db. Clearing existing dbGCellGrid. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1142) | 0205 | Deprecated output param in params file. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_conn.cpp#L1299) | 0206 | checkConnectivity error. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_graphics.cpp#L729) | 0207 | Setting MAX_THREADS=1 for use with the DR GUI. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1753) | 0210 | Layer {} minWidth is larger than width. Using width as minWidth. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L878) | 0214 | genGuides empty pin2GCellMap. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L887) | 0215 | Pin {}/{} not covered by guide. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L896) | 0216 | Pin PIN/{} not covered by guide. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L901) | 0217 | genGuides unknown type. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L938) | 0218 | Guide is not connected to design. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L941) | 0219 | Guide is not connected to design. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L986) | 0220 | genGuides_final net {} error 1. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L1005) | 0221 | genGuides_final net {} error 2. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L1013) | 0222 | genGuides_final net {} pin not in any guide. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L1062) | 0223 | Pin dangling id {} ({},{}) {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L1267) | 0224 | {} {} pin not visited, number of guides = {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L1277) | 0225 | {} {} pin not visited, fall back to feedthrough mode. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_eol.cpp#L469) | 0226 | Unsupported endofline spacing rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1132) | 0227 | Deprecated def param in params file. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L367) | 0228 | genGuides_merge cannot find touching layer. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L495) | 0229 | genGuides_split lineIdx is empty on {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L582) | 0230 | genGuides_gCell2TermMap avoid condition2, may result in guide open: {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L598) | 0231 | genGuides_gCell2TermMap avoid condition3, may result in guide open: {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L610) | 0232 | genGuides_gCell2TermMap unsupported pinfig. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L89) | 0234 | {} does not have single-cut via. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L193) | 0235 | Second layer {} does not exist. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L200) | 0236 | Updating diff-net cut spacing rule between {} and {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L218) | 0237 | Second layer {} does not exist. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L225) | 0238 | Updating same-net cut spacing rule between {} and {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L282) | 0239 | Non-rectangular shape in via definition. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L289) | 0240 | CUT layer {} does not have square single-cut via, cut layer width may be set incorrectly. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L296) | 0241 | CUT layer {} does not have single-cut via, cut layer width may be set incorrectly. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L304) | 0242 | CUT layer {} does not have default via. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L316) | 0243 | Non-rectangular shape in via definition. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L321) | 0244 | CUT layer {} has smaller width defined in LEF compared to default via. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L624) | 0245 | skipped writing guide updates to database. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L668) | 0246 | {}/{} from {} has nullptr as prefAP. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2684) | 0247 | io::Writer::fillConnFigs_net does not support this type. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_pin.cpp#L72) | 0248 | instAnalysis unsupported pinFig. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_graphics.cpp#L669) | 0249 | Net {} (id = {}). |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_graphics.cpp#L672) | 0250 | Pin {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L83) | 0251 | -param cannot be used with other arguments |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1116) | 0252 | params file is deprecated. Use tcl arguments. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_init.cpp#L171) | 0253 | Design and tech mismatch. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1723) | 0255 | Maze Route cannot find path of net {} in worker of routeBox {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L397) | 0256 | Skipping NDR {} because another rule with the same name already exists. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1575) | 0258 | Unsupported LEF58_SPACING rule for layer {} of type AREA. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1589) | 0259 | Unsupported LEF58_SPACING rule for layer {} of type SAMEMASK. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1596) | 0260 | Unsupported LEF58_SPACING rule for layer {} of type PARALLELOVERLAP. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1603) | 0261 | Unsupported LEF58_SPACING rule for layer {} of type PARALLELWITHIN. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1610) | 0262 | Unsupported LEF58_SPACING rule for layer {} of type SAMEMETALSHAREDEDGE. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1617) | 0263 | Unsupported LEF58_SPACING rule for layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1139) | 0266 | Deprecated outputTA param in params file. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/db/infra/frTime.cpp#L48) | 0267 | cpu time = {:02}:{:02}:{:02}, elapsed time = {:02}:{:02}:{:02}, memory = {:.2f} (MB), peak = {:.2f} (MB) |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA.cpp#L290) | 0268 | Done with {} horizontal wires in {} frboxes and {} vertical wires in {} frboxes. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_eol.cpp#L58) | 0269 | Unsupported endofline spacing rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_eol.cpp#L232) | 0270 | Unsupported endofline spacing rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_eol.cpp#L402) | 0271 | Unsupported endofline spacing rule. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2564) | 0272 | bottomRoutingLayer {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2576) | 0273 | topRoutingLayer {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1158) | 0274 | Deprecated threads param in params file. Use 'set_thread_count'. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_graphics.cpp#L675) | 0275 | AP ({:.5f}, {:.5f}) (layer {}) (cost {}). |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L2014) | 0276 | Valid access pattern combination not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L2802) | 0277 | Valid access pattern not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L2840) | 0278 | Valid access pattern not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1628) | 0279 | SAMEMASK unsupported for cut LEF58_SPACINGTABLE rule |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L343) | 0280 | Unknown type {} in setObjAP |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L349) | 0281 | Marker {} at ({}, {}) ({}, {}). |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L216) | 0282 | Skipping blockage. Cannot find layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1291) | 0290 | Warning: no DRC report specified, skipped writing DRC report |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1354) | 0291 | Unexpected source type in marker: {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L305) | 0292 | Marker {} at ({}, {}) ({}, {}). |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L74) | 0293 | pin name {} has no ':' delimiter |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3222) | 0294 | master {} not found in db |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3226) | 0295 | mterm {} not found in db |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3229) | 0296 | Mismatch in number of pins for term {}/{} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3255) | 0297 | inst {} not found in db |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3261) | 0298 | iterm {} not found in db |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3264) | 0299 | Mismatch in access points size {} and term pins size {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3276) | 0300 | Preferred access point is not found |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3287) | 0301 | bterm {} not found in db |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L963) | 0302 | Unsupported multiple pins on bterm {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L3294) | 0303 | Mismatch in number of pins for bterm {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L814) | 0304 | Updating design remotely failed |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L546) | 0305 | Net {} of signal type {} is not routable by TritonRoute. Move to special nets. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L566) | 0306 | Net {} of signal type {} cannot be connected to bterm {} with signal type {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L590) | 0307 | Net {} of signal type {} cannot be connected to iterm {}/{} with signal type {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L458) | 0308 | step_dr requires nine positional arguments. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1134) | 0309 | Deprecated guide param in params file. use read_guide instead. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1145) | 0310 | Deprecated outputguide param in params file. use write_guide instead. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1734) | 0311 | Unsupported branch EXCEPTMINWIDTH in PROPERTY LEF58_AREA. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1741) | 0312 | Unsupported branch EXCEPTEDGELENGTH in PROPERTY LEF58_AREA. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1746) | 0313 | Unsupported branch EXCEPTMINSIZE in PROPERTY LEF58_AREA. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1749) | 0314 | Unsupported branch EXCEPTSTEP in PROPERTY LEF58_AREA. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1752) | 0315 | Unsupported branch MASK in PROPERTY LEF58_AREA. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1755) | 0316 | Unsupported branch LAYER in PROPERTY LEF58_AREA. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2021) | 0317 | LEF58_MINIMUMCUT AREA is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2029) | 0318 | LEF58_MINIMUMCUT SAMEMETALOVERLAP is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2037) | 0319 | LEF58_MINIMUMCUT FULLYENCLOSED is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_init.cpp#L229) | 0320 | Term {} of {} contains offgrid pin shape |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_init.cpp#L239) | 0321 | Term {} of {} contains offgrid pin shape |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_init.cpp#L247) | 0322 | checkFigsOnGrid unsupported pinFig. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L2228) | 0323 | Via(s) in pin {} of {} will be ignored |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1668) | 0324 | LEF58_KEEPOUTZONE SAMEMASK is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1676) | 0325 | LEF58_KEEPOUTZONE SAMEMETAL is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1684) | 0326 | LEF58_KEEPOUTZONE DIFFMETAL is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1692) | 0327 | LEF58_KEEPOUTZONE EXTENSION is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1700) | 0328 | LEF58_KEEPOUTZONE non zero SPIRALEXTENSION is not supported. Skipping for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1560) | 0329 | Error sending INST_ROWS Job to cloud |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1721) | 0330 | Error sending UPDATE_PATTERNS Job to cloud |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA.cpp#L152) | 0331 | Error sending UPDATE_PA Job to cloud |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L1603) | 0332 | Error sending UPDATE_PA Job to cloud |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1299) | 0400 | Unsupported LEF58_SPACING rule with option EXCEPTEXACTWIDTH for layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1307) | 0401 | Unsupported LEF58_SPACING rule with option FILLCONCAVECORNER for layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1315) | 0403 | Unsupported LEF58_SPACING rule with option EQUALRECTWIDTH for layer {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1087) | 0404 | mterm {} not found in db |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1090) | 0405 | Mismatch in number of pins for term {}/{} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA_assign.cpp#L609) | 0406 | No {} tracks found in ({}, {}) for layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1872) | 0410 | frNet not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1877) | 0411 | frNet {} does not have drNets. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/ta/FlexTA_assign.cpp#L747) | 0412 | assignIroute_getDRCCost_helper overlap value is {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1079) | 0415 | Net {} already has routes. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L161) | 0416 | Term {} of {} contains offgrid pin shape. Pin shape {} is not a multiple of the manufacturing grid {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L176) | 0417 | Term {} of {} contains offgrid pin shape. Polygon point {} is not a multiple of the manufacturing grid {}. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L1198) | 0500 | Sending worker {} failed |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L169) | 0506 | -remote_host is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L174) | 0507 | -remote_port is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L179) | 0508 | -shared_volume is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L300) | 0512 | Unsupported region removeBlockObj |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/frRegionQuery.cpp#L229) | 0513 | Unsupported region addBlockObj |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L184) | 0516 | -cloud_size is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L368) | 0517 | -dump_dir is required for detailed_route_run_worker command |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L944) | 0519 | Via cut classes in LEF58_METALWIDTHVIAMAP are not supported. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L374) | 0520 | -worker_dir is required for detailed_route_run_worker command |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexGridGraph.h#L1046) | 0550 | addToByte overflow |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexGridGraph.h#L1058) | 0551 | subFromByte underflow |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L332) | 0552 | -remote_host is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L337) | 0553 | -remote_port is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L342) | 0554 | -shared_volume is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L347) | 0555 | -cloud_size is required for distributed routing. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L535) | 0606 | via in pin bottom layer {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L547) | 0607 | via in pin top layer {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_parser_helper.cpp#L55) | 0608 | Could not find user defined via {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1194) | 0610 | Load design before setting default vias |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1200) | 0611 | Via {} not found |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L477) | 0612 | -box is a list of 4 coordinates. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L484) | 0613 | -output_file is required for check_drc command |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1209) | 0615 | Load tech before setting unidirectional layers |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L1214) | 0616 | Layer {} not found |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L559) | 0617 | PDN layer {} not found. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L1216) | 0999 | Can't serialize used worker |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L103) | 1000 | Pin {} not in any guide. Attempting to patch guides to cover (at least part of) the pin. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L151) | 1001 | No guide in the pin neighborhood |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L214) | 1002 | Layer is not horizontal or vertical |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L409) | 1003 | enclosesPlanarAccess: layer is neither vertical or horizontal |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L416) | 1004 | enclosesPlanarAccess: low track not found |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L424) | 1005 | enclosesPlanarAccess: high track not found |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1771) | 1006 | failed to setTargetNet |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L101) | 1007 | PatchGuides invoked with non-term object. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L271) | 1008 | checkPinForGuideEnclosure invoked with non-term object. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_init.cpp#L1436) | 1009 | initNet_term_new invoked with non-term object. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_init.cpp#L47) | 1010 | Unsupported non-orthogonal wire begin=({}, {}) end=({}, {}), layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io.cpp#L1126) | 1011 | Access Point not found for iterm {}/{} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L741) | 12304 | Updating design remotely failed |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L2046) | 2000 | ({} {} {} coords: {} {} {}\n |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1506) | 2001 | Starting worker ({} {}) ({} {}) with {} markers |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1703) | 2002 | Routing net {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1761) | 2003 | Ending net {} with markers: |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1934) | 2005 | Creating dest search points from pins: |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1937) | 2006 | Pin {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_maze.cpp#L1968) | 2007 | ({} {} {} coords: {} {} {}\n |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.tcl#L256) | 2008 | -dump_dir is required for debugging with -dump_dr. |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/io/io_guide.cpp#L291) | 3000 | Guide in layer {} which is above max routing layer {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L79) | 4000 | DEBUGGING inst {} term {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1849) | 4500 | Edge outer dir should be either North or South |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/gc/FlexGC_main.cpp#L1861) | 4501 | Edge outer dir should be either East or West |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_graphics.cpp#L86) | 5000 | INST NOT FOUND! |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/pa/FlexPA_prep.cpp#L773) | 6000 | Macro pin has more than 1 polygon |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR_conn.cpp#L1007) | 6001 | Path segs were not split: {} and {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/dr/FlexDR.cpp#L1174) | 7461 | Balancer failed |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L457) | 9199 | Guide {} out of range {} |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L773) | 9504 | Updating globals remotely failed |
| [DRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp#L396) | 9999 | unknown update type {} |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L92) | 0001 | Worker server error: {} |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.tcl#L42) | 0002 | -host is required in run_worker cmd. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.tcl#L47) | 0003 | -port is required in run_worker cmd. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/WorkerConnection.cc#L122) | 0004 | Worker conhandler failed with message: \"{}\" |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/WorkerConnection.cc#L112) | 0005 | Unsupported job type {} from port {} |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/BalancerConnection.cc#L101) | 0006 | No workers available |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/LoadBalancer.cc#L43) | 0007 | Processed {} jobs |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/BalancerConnection.cc#L231) | 0008 | Balancer conhandler failed with message: {} |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L110) | 0009 | LoadBalancer error: {} |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.tcl#L66) | 0010 | -host is required in run_load_balancer cmd. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.tcl#L71) | 0011 | -port is required in run_load_balancer cmd. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L226) | 0012 | Serializing JobMessage failed |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L236) | 0013 | Socket connection failed with message \"{}\" |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L270) | 0014 | Sending job failed with message \"{}\" |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.tcl#L91) | 0016 | -host is required in add_worker_address cmd. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.tcl#L96) | 0017 | -port is required in add_worker_address cmd. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L279) | 0020 | Serializing result JobMessage failed |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L289) | 0022 | Sending result failed with message \"{}\" |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/WorkerConnection.cc#L78) | 0041 | Received malformed msg {} from port {} |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/BalancerConnection.cc#L86) | 0042 | Received malformed msg {} from port {} |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L165) | 0112 | Serializing JobMessage failed |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L175) | 0113 | Trial {}, socket connection failed with message \"{}\" |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L201) | 0114 | Sending job failed with message \"{}\" |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/LoadBalancer.cc#L199) | 0203 | Workers domain resolution failed with error code = {}. Message = {}. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/BalancerConnection.cc#L136) | 0204 | Exception thrown: {}. worker with ip \"{}\" and port \"{}\" will be pushed back the queue. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/BalancerConnection.cc#L146) | 0205 | Maximum of {} failing workers reached, relaying error to leader. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/BalancerConnection.cc#L209) | 0207 | {} workers failed to receive the broadcast message and have been removed. |
| [DST](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dst/src/Distributed.cc#L256) | 9999 | Problem in deserialize {} |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L180) | 0001 | Layer {} in names was not found. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L191) | 0002 | Layer {} not found. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L446) | 0003 | Filling layer {}. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L484) | 0004 | Total fills: {}. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L503) | 0005 | Filling {} areas with OPC fill. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L509) | 0006 | Total fills: {}. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/finale.tcl#L49) | 0007 | The -rules argument must be specified. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/finale.tcl#L55) | 0008 | The -area argument must be a list of 4 coordinates. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L471) | 0009 | Filling {} areas with non-OPC fill. |
| [FIN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/fin/src/DensityFill.cpp#L529) | 0010 | Skipping layer {}. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L816) | 0002 | DBU: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L843) | 0003 | SiteSize: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L844) | 0004 | CoreAreaLxLy: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L845) | 0005 | CoreAreaUxUy: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1310) | 0006 | NumInstances: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1314) | 0007 | NumPlaceInstances: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1315) | 0008 | NumFixedInstances: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1316) | 0009 | NumDummyInstances: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1317) | 0010 | NumNets: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1318) | 0011 | NumPins: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1320) | 0012 | DieAreaLxLy: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1321) | 0013 | DieAreaUxUy: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1322) | 0014 | CoreAreaLxLy: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1323) | 0015 | CoreAreaUxUy: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1329) | 0016 | CoreArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1330) | 0017 | NonPlaceInstsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1332) | 0018 | PlaceInstsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1333) | 0019 | Util(%): {:.2f} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1335) | 0020 | StdInstsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1336) | 0021 | MacroInstsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L754) | 0023 | TargetDensity: {:.2f} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L755) | 0024 | AveragePlaceInstArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L756) | 0025 | IdealBinArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L757) | 0026 | IdealBinCnt: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L758) | 0027 | TotalBinArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L788) | 0028 | BinCnt: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L793) | 0029 | BinSize: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L809) | 0030 | NumBins: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L1567) | 0031 | FillerInit: NumGCells: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L1568) | 0032 | FillerInit: NumGNets: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L1569) | 0033 | FillerInit: NumGPins: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L1915) | 0034 | gCellFiller: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L1933) | 0035 | NewTotalFillerArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L198) | 0036 | TileLxLy: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L199) | 0037 | TileSize: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L200) | 0038 | TileCnt: {} {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L201) | 0039 | numRoutingLayers: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L223) | 0040 | NumTiles: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L662) | 0045 | InflatedAreaDelta: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L663) | 0046 | TargetDensity: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L681) | 0047 | SavedMinRC: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L682) | 0048 | SavedTargetDensity: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L694) | 0049 | WhiteSpaceArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L695) | 0050 | NesterovInstsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L696) | 0051 | TotalFillerArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L697) | 0052 | TotalGCellsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L701) | 0053 | ExpectedTotalGCellsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L720) | 0054 | NewTargetDensity: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L721) | 0055 | NewWhiteSpaceArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L722) | 0056 | MovableArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L723) | 0057 | NewNesterovInstsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L725) | 0058 | NewTotalFillerArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L726) | 0059 | NewTotalGCellsArea: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L792) | 0063 | TotalRouteOverflowH2: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L793) | 0064 | TotalRouteOverflowV2: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L794) | 0065 | OverflowTileCnt2: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L849) | 0066 | 0.5%RC: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L850) | 0067 | 1.0%RC: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L851) | 0068 | 2.0%RC: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L852) | 0069 | 5.0%RC: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L854) | 0070 | 0.5rcK: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L855) | 0071 | 1.0rcK: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L856) | 0072 | 2.0rcK: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L857) | 0073 | 5.0rcK: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L865) | 0074 | FinalRC: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/routeBase.cpp#L878) | 0075 | Routability numCall: {} inflationIterCnt: {} bloatIterCnt: {} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/timingBase.cpp#L166) | 0100 | worst slack {:.3g} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/timingBase.cpp#L169) | 0102 | No slacks found. Timing-driven mode disabled. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/timingBase.cpp#L207) | 0103 | Weighted {} nets. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/timingBase.cpp#L156) | 0114 | No net slacks found. Timing-driven mode disabled. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L146) | 0115 | -disable_timing_driven is deprecated. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L158) | 0116 | -disable_routability_driven is deprecated. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L835) | 0118 | core area outside of die. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L877) | 0119 | instance {} height is larger than core. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L880) | 0120 | instance {} width is larger than core. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L118) | 0121 | No liberty libraries found. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L319) | 0130 | No rows defined in design. Use initialize_floorplan to add rows. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L383) | 0131 | No rows defined in design. Use initialize_floorplan to add rows. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.cpp#L231) | 0132 | Locked {} instances |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.cpp#L248) | 0133 | Unlocked instances |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L106) | 0134 | Master {} is not marked as a BLOCK in LEF but is more than {} rows tall. It will be treated as a macro. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L180) | 0135 | Target density must be in \[0, 1\]. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.cpp#L325) | 0136 | No placeable instances - skipping placement. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L122) | 0150 | -skip_io will disable timing driven mode. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/replace.tcl#L153) | 0151 | -skip_io will disable routability driven mode. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/initialPlace.cpp#L120) | 0250 | GPU is not available. CPU solve is being used. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/initialPlace.cpp#L125) | 0251 | CPU solver is forced to be used. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L1339) | 0301 | Utilization exceeds 100%. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L1670) | 0302 | Use a higher -density or re-floorplan with a larger core area.\nGiven target density: {:.2f}\nSuggested target density: {:.2f} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovBase.cpp#L1897) | 0303 | Use a higher -density or re-floorplan with a larger core area.\nGiven target density: {:.2f}\nSuggested target density: {:.2f} |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/nesterovPlace.cpp#L287) | 0304 | RePlAce diverged at initial iteration with steplength being {}. Re-run with a smaller init_density_penalty value. |
| [GPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gpl/src/placerBase.cpp#L829) | 0305 | Unable to find a site |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L840) | 0001 | Minimum degree: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L841) | 0002 | Maximum degree: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3330) | 0003 | Macros: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3041) | 0004 | Blockages: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L478) | 0005 | Layer $layer_name not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L389) | 0006 | Repairing antennas, iteration {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3942) | 0009 | rerouting {} nets. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/RepairAntennas.cpp#L94) | 0012 | Found {} antenna violations. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L348) | 0014 | Routed nets: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L395) | 0015 | Inserted {} diodes. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2113) | 0018 | Total wirelength: {} um |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2837) | 0019 | Found {} clock nets. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3532) | 0020 | Min routing layer: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3533) | 0021 | Max routing layer: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3534) | 0022 | Global adjustment: {}% |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3535) | 0023 | Grid origin: ({}, {}) |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/MakeWireParasitics.cpp#L200) | 0025 | Non wire or via route found on net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/MakeWireParasitics.cpp#L306) | 0026 | Missing route to pin {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/RepairAntennas.cpp#L350) | 0027 | Design has rows with different site widths. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3325) | 0028 | Found {} pins outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2933) | 0029 | Pin {} does not have geometries below the max routing layer ({}). |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1364) | 0030 | Specified layer {} for adjustment is greater than max routing layer {} and will be ignored. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2067) | 0031 | At least 2 pins in position ({}, {}), layer {}, port {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2337) | 0033 | Pin {} has invalid edge. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2886) | 0034 | Net connected to instance of class COVER added for routing. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2916) | 0035 | Pin {} is outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2976) | 0036 | Pin {} is outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3110) | 0037 | Found blockage outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3274) | 0038 | Found blockage outside die area in instance {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3309) | 0039 | Found pin {} outside die area in instance {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3365) | 0040 | Net {} has wires outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3397) | 0041 | Net {} has wires outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3001) | 0042 | Pin {} does not have geometries in a valid routing layer. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3465) | 0043 | No OR_DEFAULT vias defined. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L61) | 0044 | set_global_routing_layer_adjustment requires layer and adj arguments. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L333) | 0045 | Run global_route before repair_antennas. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L75) | 0047 | Missing dbTech. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L83) | 0048 | Command set_global_routing_region_adjustment is missing -layer argument. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L89) | 0049 | Command set_global_routing_region_adjustment is missing -adjustment argument. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L112) | 0050 | Command set_global_routing_region_adjustment needs four arguments to define a region: lower_x lower_y upper_x upper_y. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L216) | 0051 | Missing dbTech. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L220) | 0052 | Missing dbBlock. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3547) | 0053 | Routing resources analysis: |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L400) | 0054 | Using detailed placer to place {} diodes. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L231) | 0055 | Wrong number of arguments for origin. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L551) | 0056 | In argument -clock_layers, min routing layer is greater than max routing layer. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L537) | 0057 | Missing track structure for layer $layer_name. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L452) | 0059 | Missing technology file. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L464) | 0060 | Layer [$tech_layer getConstName] is greater than the max routing layer ([$max_tech_layer getConstName]). |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L467) | 0061 | Layer [$tech_layer getConstName] is less than the min routing layer ([$min_tech_layer getConstName]). |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L493) | 0062 | Input format to define layer range for $cmd is min-max. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L500) | 0063 | Missing dbBlock. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L506) | 0064 | Lower left x is outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L510) | 0065 | Lower left y is outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L514) | 0066 | Upper right x is outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L518) | 0067 | Upper right y is outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/RepairAntennas.cpp#L124) | 0068 | Global route segment not valid. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L298) | 0069 | Diode cell $diode_cell not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1028) | 0071 | Layer spacing not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1234) | 0072 | Informed region is outside die area. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L311) | 0073 | Diode cell has more than one non power/ground port. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1753) | 0074 | Routing with guides in blocked metal for net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1727) | 0075 | Connection between non-adjacent layers in net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1807) | 0076 | Net {} not properly covered. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1886) | 0077 | Segment has invalid layer assignment. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1952) | 0078 | Guides vector is empty. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2054) | 0079 | Pin {} does not have layer assignment. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2083) | 0080 | Invalid pin placement. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2468) | 0082 | Cannot find track spacing. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L512) | 0084 | Layer {} does not have a valid direction. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L507) | 0085 | Routing layer {} not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2627) | 0086 | Track for layer {} not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2672) | 0088 | Layer {:7s} Track-Pitch = {:.4f} line-2-Via Pitch: {:.4f} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2732) | 0090 | Track for layer {} not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3339) | 0094 | Design with no nets. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3599) | 0096 | Final congestion report: |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L835) | 0101 | Running extra iterations to remove overflow. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L965) | 0103 | Extra Run for hard benchmark. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L1132) | 0111 | Final number of vias: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L1133) | 0112 | Final usage 3D: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L367) | 0113 | Underflow in reduce: cap, reducedCap: {}, {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L395) | 0114 | Underflow in reduce: cap, reducedCap: {}, {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L340) | 0115 | Global routing finished with overflow. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L331) | 0118 | Routing congestion too high. Check the congestion heatmap in the GUI. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L324) | 0119 | Routing congestion too high. Check the congestion heatmap in the GUI and load {} in the DRC viewer. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/RipUp.cpp#L325) | 0121 | Route type is not maze, netID {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/RipUp.cpp#L438) | 0122 | Maze ripup wrong for net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/RipUp.cpp#L560) | 0123 | Maze ripup wrong in newRipupNet for net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2639) | 0124 | Horizontal tracks for layer {} not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L844) | 0125 | Setup heap: not maze routing. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L264) | 0146 | Argument -allow_overflow is deprecated. Use -allow_congestion. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2652) | 0147 | Vertical tracks for layer {} not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2659) | 0148 | Layer {} has invalid direction. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L1845) | 0150 | Net {} has errors during updateRouteType1. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L1879) | 0151 | Net {} has errors during updateRouteType2. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L1996) | 0152 | Net {} has errors during updateRouteType1. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L2031) | 0153 | Net {} has errors during updateRouteType2. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1574) | 0164 | Initial grid wrong y1 x1 [{} {}], net start [{} {}] routelen {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1588) | 0165 | End grid wrong y2 x2 [{} {}], net start [{} {}] routelen {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1604) | 0166 | Net {} edge[{}] maze route wrong, distance {}, i {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1618) | 0167 | Invalid 2D tree for net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L1046) | 0169 | Net {}: Invalid index for position ({}, {}). Net degree: {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L1224) | 0170 | Net {}: Invalid index for position ({}, {}). Net degree: {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze3D.cpp#L485) | 0171 | Invalid index for position ({}, {}). |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze3D.cpp#L803) | 0172 | Invalid index for position ({}, {}). |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/route.cpp#L314) | 0179 | Wrong node status {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/route.cpp#L1114) | 0181 | Wrong node status {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze3D.cpp#L1193) | 0183 | Net {}: heap underflow during 3D maze routing. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze3D.cpp#L781) | 0184 | Shift to 0 length edge, type2. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze3D.cpp#L471) | 0187 | In 3D maze routing, type 1 node shift, cnt_n1A1 is 1. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/RSMT.cpp#L140) | 0188 | Invalid number of node neighbors. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/RSMT.cpp#L143) | 0189 | Failure in copy tree. Number of edges: {}. Number of nodes: {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L266) | 0197 | Via related to pin nodes: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L267) | 0198 | Via related Steiner nodes: {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L268) | 0199 | Via filling finished. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L446) | 0200 | Start point not assigned. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/maze.cpp#L924) | 0201 | Setup heap: not maze routing. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L557) | 0202 | Target ending layer ({}) out of range. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L975) | 0203 | Caused floating pin node. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1058) | 0204 | Invalid layer value in gridsL, {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1141) | 0206 | Trying to recover a 0-length edge. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1555) | 0207 | Ripped up edge without edge length reassignment. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1563) | 0208 | Route length {}, tree length {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L2981) | 0209 | Pin {} is completely outside the die area and cannot bet routed. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L569) | 0214 | Cannot get edge capacity: edge is not vertical or horizontal. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L327) | 0215 | -ratio_margin must be between 0 and 100 percent. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L143) | 0219 | Command set_macro_extension needs one argument: extension. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L155) | 0220 | Command set_pin_offset needs one argument: offset. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/RepairAntennas.cpp#L191) | 0221 | Cannot create wire for net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L473) | 0222 | No technology has been read. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L355) | 0223 | Missing dbBlock. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L416) | 0224 | Missing dbBlock. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/RipUp.cpp#L176) | 0225 | Maze ripup wrong in newRipup. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/RipUp.cpp#L267) | 0226 | Type2 ripup not type L. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1448) | 0228 | Horizontal edge usage exceeds the maximum allowed. ({}, {}) usage={} limit={} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1464) | 0229 | Vertical edge usage exceeds the maximum allowed. ({}, {}) usage={} limit={} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/FastRoute.cpp#L1103) | 0230 | Congestion iterations cannot increase overflow, reached the maximum number of times the total overflow can be increased. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L398) | 0231 | Net name not found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3991) | 0232 | Routing congestion too high. Check the congestion heatmap in the GUI. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1514) | 0233 | Failed to open guide file {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1535) | 0234 | Cannot find net {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1555) | 0235 | Cannot find layer {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1563) | 0236 | Error reading guide file {}. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3737) | 0237 | Net {} global route wire length: {:.2f}um |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L444) | 0238 | -net is required. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3761) | 0239 | Net {} does not have detailed route. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3767) | 0240 | Net {} detailed route wire length: {:.2f}um |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L3732) | 0241 | Net {} does not have global route. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L175) | 0242 | -seed argument is required. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/RepairAntennas.cpp#L387) | 0243 | Unable to repair antennas on net with diodes. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L379) | 0244 | Diode {}/{} ANTENNADIFFAREA is zero. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.tcl#L314) | 0245 | Too arguments to repair_antennas. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L373) | 0246 | No diode with LEF class CORE ANTENNACELL found. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1417) | 0247 | v_edge mismatch {} vs {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1428) | 0248 | h_edge mismatch {} vs {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1501) | 0249 | Load design before reading guides |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L1544) | 0250 | Net {} has guides but is not routed by the global router and will be skipped. |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/GlobalRouter.cpp#L283) | 0251 | The start_incremental and end_incremental flags cannot be defined together |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1307) | 1247 | v_edge mismatch {} vs {} |
| [GRT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/grt/src/fastroute/src/utility.cpp#L1320) | 1248 | h_edge mismatch {} vs {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L47) | 0001 | Command {} is not usable in non-GUI mode |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L53) | 0002 | No database loaded |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L64) | 0003 | No database loaded |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L68) | 0005 | No chip loaded |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L72) | 0006 | No block loaded |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L337) | 0007 | Unknown display control type: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L1095) | 0008 | GUI already active. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L360) | 0009 | Unknown display control type: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui_utils.cpp#L71) | 0010 | File path does not end with a valid extension, new path is: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui_utils.cpp#L121) | 0011 | Failed to write image: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui_utils.cpp#L126) | 0012 | Image size is not valid: {}px x {}px |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/displayControls.cpp#L1041) | 0013 | Unable to find {} display control at {}. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/displayControls.cpp#L1062) | 0014 | Unable to find {} display control at {}. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L615) | 0015 | No design loaded. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L648) | 0016 | No design loaded. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L119) | 0017 | No technology loaded. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L132) | 0018 | Area must contain 4 elements. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L279) | 0019 | Display option must have 2 elements {control name} {value}. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L48) | 0020 | The -text argument must be specified. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L53) | 0021 | The -script argument must be specified. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/mainWindow.cpp#L712) | 0022 | Button {} already defined. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/mainWindow.cpp#L1196) | 0023 | Failed to open help automatically, navigate to: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/mainWindow.cpp#L1017) | 0024 | Ruler with name \"{}\" already exists |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/mainWindow.cpp#L803) | 0025 | Menu action {} already defined. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L79) | 0026 | The -text argument must be specified. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L84) | 0027 | The -script argument must be specified. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L753) | 0028 | {} is not a known map. Valid options are: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L781) | 0029 | {} is not a valid option. Valid options are: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L524) | 0030 | Unable to open TritonRoute DRC report: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L124) | 0031 | Resolution too high for design, defaulting to [expr $resolution / [$tech getLefUnits]] um per pixel |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L507) | 0032 | Unable to determine type of {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L290) | 0033 | No descriptor is registered for {}. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/displayControls.cpp#L1074) | 0034 | Found {} controls matching {} at {}. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L476) | 0035 | Unable to find descriptor for: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L497) | 0036 | Nothing selected |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.i#L503) | 0037 | Unknown property: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L160) | 0038 | Must specify -type. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L176) | 0039 | Cannot use case insensitivity without a name. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L601) | 0040 | Unable to find tech layer (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L695) | 0041 | Unable to find bterm (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L684) | 0042 | Unable to find iterm (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L673) | 0043 | Unable to find instance (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L662) | 0044 | Unable to find net (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L555) | 0045 | Unable to parse line as violation type (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L570) | 0046 | Unable to parse line as violation source (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L590) | 0047 | Unable to parse line as violation location (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L620) | 0048 | Unable to parse bounding box (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L626) | 0049 | Unable to parse bounding box (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L633) | 0050 | Unable to parse bounding box (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L722) | 0051 | Unknown source type (line: {}): {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L716) | 0052 | Unable to find obstruction (line: {}) |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L998) | 0053 | Unable to find descriptor for: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/displayControls.cpp#L766) | 0054 | Unknown data type for \"{}\". |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L762) | 0055 | Unable to parse JSON file {}: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/drcWidget.cpp#L798) | 0056 | Unable to parse violation shape: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/displayControls.cpp#L690) | 0057 | Unknown data type \"{}\" for \"{}\". |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L800) | 0060 | {} must be a boolean |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L809) | 0061 | {} must be an integer or double |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L818) | 0062 | {} must be an integer or double |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L825) | 0063 | {} must be a string |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L628) | 0064 | No design loaded. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.cpp#L633) | 0065 | No design loaded. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/heatMap.cpp#L562) | 0066 | Heat map \"{}\" has not been populated with data. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L210) | 0067 | Design not loaded. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L219) | 0068 | Pin not found. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L222) | 0069 | Multiple pin timing cones are not supported. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L253) | 0070 | Design not loaded. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/gui.tcl#L260) | 0071 | Unable to find net \"$net_name\". |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/heatMap.cpp#L94) | 0072 | \"{}\" is not populated with data. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/heatMap.cpp#L99) | 0073 | Unable to open {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/clockWidget.cpp#L1418) | 0074 | Unable to find clock: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/mainWindow.cpp#L1587) | 0076 | Unknown filetype: {} |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/mainWindow.cpp#L1581) | 0077 | Timing data is not stored in {} and must be loaded separately, if needed. |
| [GUI](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/gui/src/layoutViewer.cpp#L1915) | 0078 | Failed to write image: {} |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L474) | 0001 | Added {} rows of {} site {} with height {}. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L159) | 0010 | layer $layer_name not found. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L56) | 0011 | Unable to find site: $sitename |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L80) | 0013 | -core_space is either a list of 4 margins or one value for all margins. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L103) | 0015 | -die_area is a list of 4 coordinates. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L115) | 0016 | -core_area is a list of 4 coordinates. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L131) | 0017 | no -core_area specified. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L134) | 0019 | no -utilization or -die_area specified. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L614) | 0021 | Track pattern for {} will be skipped due to x_offset > die width. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L622) | 0022 | Track pattern for {} will be skipped due to y_offset > die height. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L162) | 0025 | layer $layer_name is not a routing layer. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L320) | 0026 | left core row: {} has less than 10 sites |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L351) | 0027 | right core row: {} has less than 10 sites |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L215) | 0028 | Core area lower left ({:.3f}, {:.3f}) snapped to ({:.3f}, {:.3f}). |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L535) | 0029 | Unable to determine tiecell ({}) function. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L560) | 0030 | Inserted {} tiecells using {}/{}. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L228) | 0031 | Unable to find master: $tie_cell |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.tcl#L233) | 0032 | Unable to find master pin: $args |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.i#L62) | 0038 | No design is loaded. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L519) | 0039 | Liberty cell or port {}/{} not found. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L429) | 0040 | Invalid height for site {} detected. The height value of {} is not a multiple of the smallest site height {}. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L180) | 0042 | No sites found. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L404) | 0043 | No site found for instance {} in block {}. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L679) | 0044 | Non horizontal layer uses property LEF58_PITCH. |
| [IFP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ifp/src/InitFloorplan.cc#L692) | 0045 | No routing Row found in layer {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/mpl.tcl#L81) | 0001 | No block found for Macro Placement. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L867) | 0002 | Floorplan has not been initialized? Pin location error for {}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L2577) | 0003 | There are no valid tilings for mixed cluster: {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L2800) | 0004 | This no valid tilings for hard macro cluser: {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L3549) | 0005 | [MultiLevelMacroPlacement] Failed on cluster: {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L3767) | 0006 | SA failed on cluster: {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L4816) | 0007 | Not enough space in cluster: {} for child hard macro cluster: {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L4839) | 0008 | Not enough space in cluster: {} for child mixed cluster: {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L4989) | 0009 | Fail !!! Bus planning error !!! |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L5179) | 0010 | Cannot find valid macro placement for hard macro cluster: {} |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L1287) | 0011 | Pin {} is not placed, using west. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L709) | 0012 | Unhandled partition class. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/bus_synthesis.cpp#L1110) | 0013 | bus planning - error condition nullptr |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/bus_synthesis.cpp#L1124) | 0014 | bus planning - error condition pre_cluster |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/graphics.cpp#L230) | 0015 | Unexpected orientation |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L432) | 0061 | Parquet area {:g} x {:g} exceeds the partition area {:g} x {:g}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L232) | 0066 | Partitioning failed. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L208) | 0067 | Initial weighted wire length {:g}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L230) | 0068 | Placed weighted wire length {:g}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L279) | 0069 | Initial weighted wire length {:g}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L404) | 0070 | Using {} partition sets. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L452) | 0071 | Solution {} weighted wire length {:g}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L489) | 0072 | No partition solutions found. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L482) | 0073 | Best weighted wire length {:g}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L575) | 0076 | Partition {} macros. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L628) | 0077 | Using {} cut lines. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L640) | 0079 | Cut line {:.2f}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L776) | 0080 | Impossible partition found. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.tcl#L85) | 0085 | fence_region outside of core area. Using core area. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.tcl#L69) | 0089 | No rows found. Use initialize_floorplan to add rows. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.tcl#L49) | 0092 | -halo receives a list with 2 values, [llength $halo] given. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.tcl#L60) | 0093 | -channel receives a list with 2 values, [llength $channel] given. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.tcl#L80) | 0094 | -fence_region receives a list with 4 values, [llength $fence_region] given. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.tcl#L102) | 0095 | Snap layer $snap_layer is not a routing layer. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.tcl#L115) | 0096 | Unknown placement style. Use one of corner_max_wl or corner_min_wl. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L522) | 0097 | Unable to find a site |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L164) | 0098 | Some instances do not have Liberty models. TritonMP will place macros without connection information. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L808) | 0099 | Macro {} is unplaced, use global_placement to get an initial placement before macro placement. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L829) | 0100 | No macros found. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L833) | 0101 | Found {} macros. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl/src/MacroPlacer.cpp#L196) | 0102 | {} pins {}. |
| [MPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/mpl2/src/hier_rtlmp.cpp#L4710) | 2067 | [MultiLevelMacroPlacement] Failed on cluster {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1438) | 0000 | {} Net {} not found |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbDatabase.cpp#L672) | 0002 | Error opening file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L916) | 0004 | Hierarchical block information is lost |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L1913) | 0005 | Can not find master {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L1938) | 0006 | Can not find net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L1963) | 0007 | Can not find inst {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L2549) | 0008 | Cannot duplicate net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L2553) | 0009 | id mismatch ({},{}) for net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L2910) | 0010 | tot = {}, upd = {}, enc = {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3054) | 0011 | net {} {} capNode {} ccseg {} has otherCapNode {} not from changed or halo nets |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3064) | 0012 | the other capNode is from net {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3219) | 0013 | ccseg {} has other capn {} not from changed or halo nets |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3393) | 0014 | Failed to generate non-default rule for single wire net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn_w.cpp#L116) | 0015 | disconnected net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1719) | 0016 | problem in getExtension() |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3490) | 0017 | Failed to write def file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1998) | 0018 | error in addToWire |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3511) | 0019 | Can not open file {} to write! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3518) | 0020 | Memory allocation failed for io buffer |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbCCSeg.cpp#L408) | 0021 | ccSeg={} capn0={} next0={} capn1={} next1={} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbCCSeg.cpp#L434) | 0022 | ccSeg {} has capnd {} {}, not {} ! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbCCSeg.cpp#L755) | 0023 | CCSeg {} does not have orig capNode {}. Can not swap. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbCapNode.cpp#L182) | 0024 | cc seg {} has both capNodes {} {} from the same net {} . ignored by groundCC . |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbCapNode.cpp#L1100) | 0025 | capn {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbITerm.cpp#L679) | 0034 | Can not find physical location of iterm {}/{} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L690) | 0036 | setLevel {} greater than 255 is illegal! inst {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1000) | 0037 | instance bound to a block {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1021) | 0038 | instance already bound to a block {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1026) | 0039 | Forced Initialize to 0 |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1034) | 0040 | block already bound to an instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1039) | 0041 | Forced Initialize to 0 |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1047) | 0042 | block not a direct child of instance owner |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1056) | 0043 | _dbHier::create fails |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1167) | 0044 | Failed(_hierarchy) to swap: {} -> {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1215) | 0045 | Failed(termSize) to swap: {} -> {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1242) | 0046 | Failed(mtermEquiv) to swap: {} -> {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1552) | 0047 | instance {} has no output pin |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbNet.cpp#L1013) | 0048 | Net {} {} had been CC adjusted by {}. Please unadjust first. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbNet.cpp#L2096) | 0049 | Donor net {} {} has no rc data |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbNet.cpp#L2102) | 0050 | Donor net {} {} has no capnode attached to iterm {}/{} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbNet.cpp#L2126) | 0051 | Receiver net {} {} has no capnode attached to iterm {}/{} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbNet.cpp#L2541) | 0052 | Net {}, {} has no extraction data |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbNet.cpp#L2587) | 0053 | Net {}, {} has no extraction data |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbRSeg.cpp#L553) | 0054 | CC segs of RSeg {}-{} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbRSeg.cpp#L561) | 0055 | CC{} : {}-{} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbRSeg.cpp#L568) | 0056 | rseg {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbRSeg.cpp#L845) | 0057 | Cannot find cap nodes for Rseg {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbTech.cpp#L955) | 0058 | Layer {} is not a routing layer! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbTech.cpp#L964) | 0059 | Layer {}, routing level {}, has {} pitch !! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbTech.cpp#L973) | 0060 | Layer {}, routing level {}, has {} width !! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbTech.cpp#L982) | 0061 | Layer {}, routing level {}, has {} spacing !! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWire.cpp#L1897) | 0062 | This wire has no net |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1179) | 0063 | {} No wires for net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1188) | 0064 | {} begin decoder for net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1197) | 0065 | {} End decoder for net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1209) | 0066 | {} New path: layer {} type {} non-default rule {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1218) | 0067 | {} New path: layer {} type {}\n |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1241) | 0068 | {} New path at junction {}, point(ext) {} {} {}, with rule {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1253) | 0069 | {} New path at junction {}, point(ext) {} {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1265) | 0070 | {} New path at junction {}, point {} {}, with rule {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1275) | 0071 | {} New path at junction {}, point {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1284) | 0072 | {} opcode after junction is not point or point_ext??\n |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1303) | 0073 | {} Short at junction {}, with rule {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1310) | 0074 | {} Short at junction {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1325) | 0075 | {} Virtual wire at junction {}, with rule {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1332) | 0076 | {} Virtual wire at junction {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1339) | 0077 | {} Found point {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1345) | 0078 | {} Found point(ext){} {} {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1351) | 0079 | {} Found via {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1360) | 0080 | block via found in signal net! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1374) | 0081 | {} Found Iterm |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1379) | 0082 | {} Found Bterm |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1387) | 0083 | {} GOT RULE {}, EXPECTED RULE {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1396) | 0084 | {} Found Rule {} in middle of path |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1405) | 0085 | {} End decoder for net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L1413) | 0086 | {} Hit default! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWirePathItr.cpp#L515) | 0087 | {} No wires for net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definBlockage.cpp#L72) | 0088 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definBlockage.cpp#L83) | 0089 | error: undefined component ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definBlockage.cpp#L196) | 0090 | error: undefined component ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definBlockage.cpp#L217) | 0091 | warning: Blockage max density {} not in [0, 100] will be ignored |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definComponent.cpp#L151) | 0092 | error: unknown library cell referenced ({}) for instance ({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definComponent.cpp#L173) | 0093 | error: duplicate instance definition({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definComponent.cpp#L179) | 0094 | \t\tCreated {} Insts |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definFill.cpp#L64) | 0095 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L122) | 0096 | net {} does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L147) | 0097 | \t\tCreated {} Nets |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L161) | 0098 | duplicate must-join net found ({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L200) | 0099 | error: netlist component ({}) is not defined |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L210) | 0100 | error: netlist component-pin ({}, {}) is not defined |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L247) | 0101 | error: undefined NONDEFAULTRULE ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L264) | 0102 | error: NONDEFAULTRULE ({}) of net ({}) does not match DEF rule ({}). |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L278) | 0103 | error: undefined NONDEFAULTRULE ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L363) | 0104 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L378) | 0105 | error: RULE ({}) referenced for layer ({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L404) | 0106 | error: undefined TAPER RULE ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L518) | 0107 | error: undefined via ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L550) | 0108 | error: invalid VIA layers, cannot determine exit layer of path |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L574) | 0109 | error: undefined via ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNet.cpp#L591) | 0110 | error: invalid VIA layers in {} in net {}, currently on layer {} at ({}, {}), cannot determine exit layer of path |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNonDefaultRule.cpp#L64) | 0111 | error: Duplicate NONDEFAULTRULE {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNonDefaultRule.cpp#L85) | 0112 | error: Cannot find tech-via {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNonDefaultRule.cpp#L101) | 0113 | error: Cannot find tech-via-generate rule {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNonDefaultRule.cpp#L118) | 0114 | error: Cannot find layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNonDefaultRule.cpp#L134) | 0115 | error: Cannot find layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNonDefaultRule.cpp#L142) | 0116 | error: Duplicate layer rule ({}) in non-default-rule statement. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definPin.cpp#L118) | 0117 | PIN {} missing right bus character. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definPin.cpp#L124) | 0118 | PIN {} missing left bus character. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definPin.cpp#L202) | 0119 | error: Cannot specify effective width and minimum spacing together. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definPin.cpp#L217) | 0120 | error: Cannot specify effective width and minimum spacing together. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definPin.cpp#L237) | 0121 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definPin.cpp#L372) | 0122 | error: Cannot find PIN {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definPin.cpp#L385) | 0123 | error: Cannot find PIN {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L537) | 0124 | warning: Polygon DIEAREA statement not supported. The bounding box will be used instead |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1610) | 0125 | lines processed: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1615) | 0126 | error: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1641) | 0127 | Reading DEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L353) | 0128 | Design: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1645) | 0129 | Error: Failed to read DEF file |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1650) | 0130 | Created {} pins. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1656) | 0131 | Created {} components and {} component-terminals. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1667) | 0132 | Created {} special nets and {} connections. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1674) | 0133 | Created {} nets and {} connections. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1687) | 0134 | Finished DEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1699) | 0135 | Reading DEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1703) | 0137 | Error: Failed to read DEF file |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1708) | 0138 | Created {} pins. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1711) | 0139 | Created {} components and {} component-terminals. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1718) | 0140 | Created {} special nets and {} connections. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1725) | 0141 | Created {} nets and {} connections. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1731) | 0142 | Finished DEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1742) | 0143 | Reading DEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1746) | 0144 | Error: Failed to read DEF file |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1751) | 0145 | Processed {} special nets. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1755) | 0146 | Processed {} nets. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1757) | 0147 | Finished DEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1827) | 0148 | error: Cannot open DEF file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1848) | 0149 | DEF parser returns an error! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1863) | 0150 | error: Cannot open DEF file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1884) | 0151 | DEF parser returns an error! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definRegion.cpp#L65) | 0152 | Region \"{}\" already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definRow.cpp#L111) | 0155 | error: undefined site ({}) referenced in row ({}) statement. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L123) | 0156 | special net {} does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L178) | 0157 | error: netlist component ({}) is not defined |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L188) | 0158 | error: netlist component-pin ({}, {}) is not defined |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L243) | 0159 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L262) | 0160 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L297) | 0161 | error: SHIELD net ({}) does not exists. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L315) | 0162 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L442) | 0163 | error: undefined ia ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definSNet.cpp#L499) | 0164 | error: undefined ia ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definTracks.cpp#L70) | 0165 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L67) | 0166 | error: duplicate via ({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L80) | 0167 | error: cannot file VIA GENERATE rule in technology ({}). |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L116) | 0168 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L125) | 0169 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L134) | 0170 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L231) | 0171 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defout/defout_impl.cpp#L151) | 0172 | Cannot open DEF file ({}) for writing |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defout/defout_impl.cpp#L910) | 0173 | warning: pin {} skipped because it has no net |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defout/defout_impl.cpp#L1622) | 0174 | warning: missing shield net |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L182) | 0175 | illegal: non-orthogonal-path at Pin |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L204) | 0176 | error: undefined layer ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L390) | 0177 | error: undefined via ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L410) | 0178 | error: undefined via ({}) referenced |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L523) | 0179 | invalid BUSBITCHARS ({})\n |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L601) | 0180 | duplicate LAYER ({}) ignored |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L622) | 0181 | Skipping LAYER ({}) ; Non Routing or Cut type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L631) | 0182 | Skipping LAYER ({}) ; cannot understand type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L824) | 0183 | In layer {}, spacing layer {} not found |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1191) | 0184 | cannot find EEQ for macro {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1200) | 0185 | cannot find LEQ for macro {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1245) | 0186 | macro {} references unknown site {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1295) | 0187 | duplicate NON DEFAULT RULE ({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1306) | 0188 | Invalid layer name {} in NON DEFAULT RULE {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1343) | 0189 | Invalid layer name {} in NONDEFAULT SPACING |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1351) | 0190 | Invalid layer name {} in NONDEFAULT SPACING |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1374) | 0191 | error: undefined VIA {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1387) | 0192 | error: undefined VIA GENERATE RULE {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1401) | 0193 | error: undefined LAYER {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1455) | 0194 | Cannot add a new PIN ({}) to MACRO ({}), because the pins have already been defined. \n |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1480) | 0195 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1497) | 0196 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1514) | 0197 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1531) | 0198 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1557) | 0199 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1573) | 0200 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1589) | 0201 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1606) | 0202 | Invalid layer name {} in antenna info for term {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1701) | 0203 | Invalid layer name {} in SPACING |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1707) | 0204 | Invalid layer name {} in SPACING |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1750) | 0205 | The LEF UNITS DATABASE MICRON convert factor ({}) is greater than the database units per micron ({}) of the current technology. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1780) | 0206 | error: invalid dbu-per-micron value {}; valid units (100, 200, 400, 8001000, 2000, 4000, 8000, 10000, 20000) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1797) | 0207 | Unknown object type for USEMINSPACING: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1815) | 0208 | VIA: duplicate VIA ({}) ignored... |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1848) | 0209 | VIA: undefined layer ({}) in VIA ({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1878) | 0210 | error: missing VIA GENERATE rule {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1894) | 0211 | error: missing LAYER {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1903) | 0212 | error: missing LAYER {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1911) | 0213 | error: missing LAYER {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1965) | 0214 | duplicate VIARULE ({}) ignoring... |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1975) | 0215 | error: VIARULE ({}) undefined layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2005) | 0216 | error: undefined VIA {} in VIARULE {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2024) | 0217 | duplicate VIARULE ({}) ignoring... |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2034) | 0218 | error: VIARULE ({}) undefined layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2102) | 0221 | {} lines parsed! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2107) | 0222 | Reading LEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2184) | 0223 | Created {} technology layers |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2188) | 0224 | Created {} technology vias |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2190) | 0225 | Created {} library cells |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2192) | 0226 | Finished LEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2203) | 0227 | Error: technology already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2232) | 0228 | Error: technology does not exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2237) | 0229 | Error: library ({}) already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2267) | 0230 | Error: library ({}) already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2273) | 0231 | Error: technology already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2313) | 0232 | Error: library ({}) already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2319) | 0233 | Error: technology already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2335) | 0234 | Reading LEF file: {} ... |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2337) | 0235 | Error reading {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2347) | 0236 | Finished LEF file: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2351) | 0237 | Created {} technology layers |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2355) | 0238 | Created {} technology vias |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2358) | 0239 | Created {} library cells |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/reader.cpp#L557) | 0240 | error: Cannot open LEF file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/create_box.cpp#L154) | 0241 | error: Cannot create a via instance, via ({}) has no shapes |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/create_box.cpp#L183) | 0242 | error: Can not determine which direction to continue path, |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/create_box.cpp#L186) | 0243 | via ({}) spans above and below the current layer ({}). |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/create_box.cpp#L218) | 0244 | error: Cannot create a via instance, via ({}) has no shapes |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/create_box.cpp#L247) | 0245 | error: Net {}: Can not determine which direction to continue path, |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2175) | 0246 | unknown incomplete layer prop of type {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L927) | 0247 | skipping undefined pin {} encountered in {} DEF |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L458) | 0248 | skipping undefined comp {} encountered in {} DEF |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1454) | 0249 | skipping undefined net {} encountered in FLOORPLAN DEF |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1632) | 0250 | Chip does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1653) | 0252 | Updated {} pins. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1663) | 0253 | Updated {} components. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1681) | 0254 | Updated {} nets and {} connections. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L69) | 0260 | DESIGN is not defined in DEF |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L330) | 0261 | Block with name \"{}\" already exists, renaming too \"{}\" |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/reader.cpp#L548) | 0270 | error: Cannot open zipped LEF file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1836) | 0271 | error: Cannot open zipped DEF file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L929) | 0273 | Create ND RULE {} for layer/width {},{} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/create_box.cpp#L59) | 0274 | Zero length path segment ({},{}) ({},{}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L691) | 0275 | skipping undefined net {} encountered in FLOORPLAN DEF |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/create_box.cpp#L252) | 0276 | via ({}) spans above and below the current layer ({}). |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2119) | 0277 | dropping LEF58_SPACING rule for cut layer {} for referencing undefined layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L729) | 0279 | parse mismatch in layer property {} for layer {} : \"{}\" |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2135) | 0280 | dropping LEF58_SPACINGTABLE rule for cut layer {} for referencing undefined layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbTechLayer.cpp#L1861) | 0282 | setNumMask {} not in range [1,3] |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/cdl/cdl.cpp#L113) | 0283 | Can't open masters file {}. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/cdl/cdl.cpp#L144) | 0284 | Master {} not found. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/cdl/cdl.cpp#L148) | 0285 | Master {} seen more than once in {}. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/cdl/cdl.cpp#L159) | 0286 | Terminal {} of CDL master {} not found in LEF. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/cdl/cdl.cpp#L220) | 0287 | Master {} was not in the masters CDL files. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2217) | 0288 | LEF data from {} is discarded due to errors |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2294) | 0289 | LEF data from {} is discarded due to errors |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2364) | 0290 | LEF data discarded due to errors |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2342) | 0291 | LEF data from {} is discarded due to errors |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2254) | 0292 | LEF data from {} is discarded due to errors |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1297) | 0293 | TECHNOLOGY is ignored |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbObject.cpp#L91) | 0294 | dbInstHdrObj not expected in getDbName |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbObject.cpp#L191) | 0295 | dbHierObj not expected in getDbName |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbObject.cpp#L377) | 0296 | dbNameObj not expected in getDbName |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbModule.cpp#L196) | 0297 | Physical only instance {} can't be added to module {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbModule.cpp#L310) | 0298 | The top module can't be destroyed. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L257) | 0299 | Via {} has only {} shapes and must have at least three. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L267) | 0300 | Via {} has cut top layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L277) | 0301 | Via {} has cut bottom layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definVia.cpp#L293) | 0302 | Via {} has no cut shapes. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/zutil/util.cpp#L229) | 0303 | The initial {} rows ({} sites) were cut with {} shapes for a total of {} rows ({} sites). |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definGroup.cpp#L61) | 0304 | Group \"{}\" already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definGroup.cpp#L74) | 0305 | Region \"{}\" is not found |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definGroup.cpp#L99) | 0306 | error: netlist component ({}) is not defined |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L3549) | 0307 | Guides file could not be opened. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L10) | 0308 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L15) | 0309 | duplicate group name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L27) | 0310 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L35) | 0311 | please define either top module or the modinst path |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L38) | 0312 | module does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L47) | 0313 | duplicate group name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L59) | 0314 | duplicate group name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L196) | 0315 | -area is a list of 4 coordinates |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L204) | 0316 | please define area |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L211) | 0317 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L216) | 0318 | duplicate region name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L223) | 0319 | duplicate group name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L236) | 0320 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L241) | 0321 | group does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L244) | 0322 | group is not of physical cluster type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L257) | 0323 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L262) | 0324 | group does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L265) | 0325 | group is not of voltage domain type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L277) | 0326 | define domain name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L282) | 0327 | define net name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L287) | 0328 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L293) | 0329 | group does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L296) | 0330 | group is not of voltage domain type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L299) | 0331 | net does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L311) | 0332 | define domain name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L316) | 0333 | define net name |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L321) | 0334 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L327) | 0335 | group does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L330) | 0336 | group is not of voltage domain type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L333) | 0337 | net does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L347) | 0338 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L352) | 0339 | cluster does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L355) | 0340 | group is not of physical cluster type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L360) | 0341 | modinst does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L367) | 0342 | inst does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L374) | 0343 | child physical cluster does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L377) | 0344 | child group is not of physical cluster type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L392) | 0345 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L397) | 0346 | cluster does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L400) | 0347 | group is not of physical cluster type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L405) | 0348 | parent module does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L409) | 0349 | modinst does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L416) | 0350 | inst does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L423) | 0351 | child physical cluster does not exist |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L426) | 0352 | child group is not of physical cluster type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L438) | 0353 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L456) | 0354 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L519) | 0355 | please load the design before trying to use this command |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2151) | 0356 | dropping LEF58_METALWIDTHVIAMAP for referencing undefined layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/cdl/cdl.cpp#L214) | 0357 | Master {} was not in the masters CDL files, but master has no pins. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/cdl/cdl.cpp#L189) | 0358 | cannot open file {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L478) | 0359 | Attempt to change the origin of {} instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L572) | 0360 | Attempt to change the orientation of {} instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L2165) | 0361 | dropping LEF58_AREA for referencing undefined layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1424) | 0362 | Attempt to destroy dont_touch instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbNet.cpp#L2944) | 0364 | Attempt to destroy dont_touch net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbModule.cpp#L209) | 0367 | Attempt to change the module of dont_touch instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1159) | 0368 | Attempt to change master of dont_touch instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbITerm.cpp#L468) | 0369 | Attempt to connect iterm of dont_touch instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbITerm.cpp#L530) | 0370 | Attempt to disconnect iterm of dont_touch instance {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbModule.cpp#L244) | 0371 | Attempt to remove dont_touch instance {} from parent module |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbITerm.cpp#L541) | 0372 | Attempt to disconnect iterm of dont_touch net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbITerm.cpp#L476) | 0373 | Attempt to connect iterm to dont_touch net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBTerm.cpp#L678) | 0374 | Attempt to destroy bterm on dont_touch net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBTerm.cpp#L449) | 0375 | Attempt to disconnect bterm of dont_touch net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBTerm.cpp#L623) | 0376 | Attempt to create bterm on dont_touch net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBTerm.cpp#L415) | 0377 | Attempt to connect bterm to dont_touch net {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L4051) | 0378 | Global connections are not set up. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbGlobalConnect.cpp#L320) | 0379 | {} is marked do not touch, will be skipped for global conenctions |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbGlobalConnect.cpp#L349) | 0380 | {}/{} is connected to {} which is marked do not touch, this connection will not be modified. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L4025) | 0381 | Invalid net specified. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L4029) | 0382 | {} is marked do not touch, which will cause the global connect rule to be ignored. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBlock.cpp#L4097) | 0383 | {} is marked do not touch and will be skipped in global connections. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbGlobalConnect.cpp#L280) | 0384 | Invalid regular expression specified the {} pattern: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbInst.cpp#L1337) | 0385 | Attempt to create instance with duplicate name: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/zutil/util.cpp#L213) | 0386 | {} contains {} placed instances and will not be cut. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definNonDefaultRule.cpp#L224) | 0387 | error: Non-default rule ({}) has no rule for layer {}. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L736) | 0388 | unsupported {} property for layer {} :\"{}\" |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L92) | 0389 | Cannot allocate {} MBytes for mapArray |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1268) | 0390 | order_wires failed: net {}, shorts to another term at wire point ({} {}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1297) | 0391 | order_wires failed: net {}, shorts to another term at wire point ({} {}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1332) | 0392 | order_wires failed: net {}, shorts to another term at wire point ({} {}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1874) | 0393 | tmg_conn::addToWire: value of k is negative: {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbSearch.cpp#L1542) | 0394 | Cannot find instance in DB with id {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1626) | 0395 | cannot order {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L1698) | 0396 | cannot order {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbSearch.cpp#L1551) | 0397 | Cannot find instance in DB with name {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L735) | 0398 | Cannot open file {} for writting |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L777) | 0399 | Cannot open file {} for writting |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1022) | 0400 | Cannot create wire, because net name is NULL\n |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1024) | 0401 | Cannot create wire, because routing layer ({}) is invalid |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1073) | 0402 | Cannot create net %s, because wire width ({}) is less than minWidth ({}) on layer {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1099) | 0403 | Cannot create net {}, because failed to create bterms |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1332) | 0404 | Cannot create wire, because net name is NULL |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1334) | 0405 | Cannot create wire, because routing layer ({}) is invalid |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1408) | 0406 | Cannot create net {}, duplicate net |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1422) | 0407 | Cannot create net {}, because failed to create bterms |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1533) | 0408 | Cannot create wire, because net name is NULL |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1540) | 0409 | Cannot create net {}, duplicate net |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1562) | 0410 | Cannot create wire, because routing layer ({}) is invalid |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1570) | 0411 | Cannot create wire, because routing layer ({}) is invalid |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1596) | 0412 | Cannot create wire, because routing layer ({}) is invalid |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L1604) | 0413 | Cannot create wire, because routing layer ({}) is invalid |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbSearch.cpp#L1589) | 0414 | Cannot find instance term in DB with id {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbSearch.cpp#L2085) | 0415 | Cannot find net in DB with id {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbSearch.cpp#L2157) | 0416 | Cannot find block term in DB with id {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbUtil.cpp#L131) | 0417 | There is already an ECO block present! Will continue updating |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbSearch.cpp#L2101) | 0418 | Cannot find net in DB with name {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbSearch.cpp#L2185) | 0419 | Cannot find block term in DB with name {} |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/tmg_conn.cpp#L646) | 0420 | tmg_conn::detachTilePins: tilepin inside iterm. |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1846) | 0421 | DEF parser returns an error! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/defin/definReader.cpp#L1882) | 0422 | DEF parser returns an error! |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L607) | 0423 | LEF58_REGION layer {} ignored |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/zutil/parse.cpp#L65) | 0424 | Cannot zero/negative number of chars |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/zutil/parse.cpp#L50) | 0428 | Cannot open file {} for \"{}\" |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/zutil/parse.cpp#L412) | 0429 | Syntax Error at line {} ({}) |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbBox.cpp#L757) | 0430 | Layer {} has index {} which is too large to be stored |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbMaster.cpp#L739) | 0431 | Can't delete master {} which still has instances |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L69) | 1000 | Layer ${layerName} not found, skipping NDR for this layer |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L137) | 1001 | Layer ${firstLayer} not found |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L140) | 1002 | Layer ${lastLayer} not found |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L152) | 1004 | -name is missing |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L159) | 1005 | NonDefaultRule ${name} already exists |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L164) | 1006 | Spacing values \[$spacings\] are malformed |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L171) | 1007 | Width values \[$widths\] are malformed |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L179) | 1008 | Via ${viaName} not found, skipping NDR for this via |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/odb.tcl#L89) | 1009 | Invalid input in create_ndr cmd |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbAccessPoint.cpp#L323) | 1100 | Access direction is of unknown type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbAccessPoint.cpp#L343) | 1101 | Access direction is of unknown type |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/db/dbWireCodec.cpp#L630) | 1102 | Mask color: {}, but must be between 1 and 3 |
| [ODB](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/odb/src/lefin/lefin.cpp#L1175) | 2000 | Cannot parse LEF property '{}' with value '{}' |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L45) | 0001 | $filename does not exist. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L48) | 0002 | $filename is not readable. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L71) | 0003 | $filename does not exist. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L74) | 0004 | $filename is not readable. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L77) | 0005 | No technology has been read. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L107) | 0006 | DEF versions 5.8, 5.7, 5.6, 5.5, 5.4, 5.3 supported. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L172) | 0007 | $filename does not exist. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L175) | 0008 | $filename is not readable. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L325) | 0013 | Command units uninitialized. Use the read_liberty or set_cmd_units command to set units. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Metrics.tcl#L64) | 0014 | both -setup and -hold specified. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.i#L404) | 0015 | Unknown tool name {} |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L87) | 0016 | Options -incremental, -floorplan_initialization, and -child are mutually exclusive. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Metrics.tcl#L81) | 0017 | both -setup and -hold specified. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Metrics.tcl#L98) | 0018 | both -setup and -hold specified. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Metrics.tcl#L48) | 0019 | both -setup and -hold specified. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.cc#L551) | 0030 | Using {} thread(s). |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.cc#L537) | 0031 | Unable to determine maximum number of threads.\nOne thread will be used. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.cc#L568) | 0032 | Invalid thread number specification: {}. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L80) | 0033 | -order_wires is deprecated. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.cc#L383) | 0034 | More than one lib exists, multiple files will be written. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Design.cc#L64) | 0036 | A block already exists in the db |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Design.cc#L124) | 0037 | No block loaded. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Main.cc#L265) | 0038 | -gui is not yet supported with -python |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Main.cc#L269) | 0039 | .openroad ignored with -python |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L271) | 0041 | The flags -power and -ground of the add_global_connection command are mutually exclusive. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L275) | 0042 | The -net option of the add_global_connection command is required. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L283) | 0043 | The -pin_pattern option of the add_global_connection command is required. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L290) | 0044 | Net created for $keys(-net), if intended as power or ground net add the -power/-ground switch as appropriate. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L312) | 0045 | Region \"$keys(-region)\" not defined |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L304) | 0046 | -defer_connection has been deprecated. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.cc#L432) | 0047 | You can't load a new db file as the db is already populated |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.cc#L304) | 0048 | You can't load a new DEF file as the db is already populated. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Design.cc#L80) | 0101 | Only one of the options -incremental and -floorplan_init can be set at a time |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/Design.cc#L86) | 0102 | No technology has been read. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.cc#L473) | 0105 | Can't open {} |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L48) | 0201 | Use -layer or -via but not both. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L57) | 0202 | layer $layer_name not found. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L61) | 0203 | $layer_name is not a routing layer. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L65) | 0204 | missing -capacitance or -resistance argument. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L98) | 0205 | via $layer_name not found. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L102) | 0206 | -capacitance not supported for vias. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L120) | 0208 | no -resistance specified for via. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L123) | 0209 | missing -layer or -via argument. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L193) | 1009 | -name is missing. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L196) | 1010 | Either -net or -all_clocks need to be defined. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L202) | 1011 | No NDR named ${ndrName} found. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L208) | 1012 | No net named ${netName} found. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L155) | 1013 | -masters is required. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/OpenRoad.tcl#L138) | 1050 | Options -bloat and -bloat_occupied_layers are both set. At most one should be used. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L918) | 2001 | LEF macro {} pin {} missing from liberty cell. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1040) | 2002 | Liberty cell {} pin {} missing from LEF macro. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1217) | 2003 | deletePin not implemented for dbITerm |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1249) | 2004 | unimplemented network function mergeInto |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1254) | 2005 | unimplemented network function mergeInto |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1318) | 2006 | pin is not ITerm or BTerm |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1375) | 2007 | unhandled port direction |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1447) | 2008 | unknown master term type |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbReadVerilog.tcl#L51) | 2009 | missing top_cell_name argument and no current_design. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbReadVerilog.tcl#L58) | 2010 | no technology has been read. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbReadVerilog.cc#L425) | 2011 | LEF master {} has no liberty cell. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbReadVerilog.cc#L431) | 2012 | Liberty cell {} has no LEF master. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbReadVerilog.cc#L293) | 2013 | instance {} LEF master {} not found. |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbReadVerilog.cc#L275) | 2014 | hierachical instance creation failed for {} of {} |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbReadVerilog.cc#L302) | 2015 | leaf instance creation failed for {} of {} |
| [ORD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbNetwork.cc#L1290) | 2016 | instance is not Inst or ModInst |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L567) | 0001 | Unable to place {} ({}) at ({:.3f}um, {:.3f}um) - ({:.3f}um, {:.3f}um) as it overlaps with {} ({}) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L971) | 0002 | {}/{} ({}) and {}/{} ({}) are touching, but are connected to different nets |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L119) | 0003 | {:.3f}um is below the minimum width for {}, changing to {:.3f}um |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L133) | 0004 | {:.3f}um is below the minimum spacing for {}, changing to {:.3f}um |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L184) | 0005 | Routing {} nets |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L230) | 0006 | Failed to route the following {} nets: |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L247) | 0007 | Failed to route {} nets. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L294) | 0008 | Unable to snap ({:.3f}um, {:.3f}um) to routing grid. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L419) | 0009 | No edges added to routing grid to access ({:.3f}um, {:.3f}um). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/RDLRouter.cpp#L1150) | 0010 | {} only has one iterm on {} layer |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L76) | 0011 | {} is not of type {}, but is instead {} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L90) | 0012 | {} is not of type {}, but is instead {} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L414) | 0013 | Unable to find {} row to place a corner cell in |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L263) | 0014 | Horizontal site must be speficied. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L266) | 0015 | Vertical site must be speficied. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L269) | 0016 | Corner site must be speficied. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L448) | 0018 | Unable to create instance {} without master |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L455) | 0019 | Row must be specified to place a pad |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L596) | 0020 | Row must be specified to place IO filler |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L774) | 0021 | Row must be specified to remove IO filler |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L1165) | 0022 | Layer must be specified to perform routing. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L73) | 0023 | Master must be specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L181) | 0024 | Instance must be specified to assign it to a bump. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L186) | 0025 | Net must be specified to assign it to a bump. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L678) | 0026 | Filling {} ({:.3f}um -> {:.3f}um) will result in a gap. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L799) | 0027 | Bond master must be specified to place bond pads |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L407) | 0028 | Corner master must be specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L516) | 0029 | {} is not a recognized IO row. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L754) | 0030 | Unable to fill gap completely {:.3f}um -> {:.3f}um in row {} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L835) | 0031 | {} contains more than 1 pin shape on {} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L829) | 0032 | Unable to determine the top layer of {} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L159) | 0033 | Could not find a block terminal associated with net: \"{}\", creating now. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.cpp#L166) | 0034 | Unable to create block terminal: {} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L388) | 0100 | Unable to find site: $name |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L396) | 0101 | Unable to find master: $name |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L404) | 0102 | Unable to find instance: $name |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L412) | 0103 | Unable to find net: $name |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L420) | 0104 | $arg is required for $cmd |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L347) | 0105 | Unable to find layer: $keys(-layer) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.i#L150) | 0106 | Unable to find row: {} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L353) | 0107 | Unable to find techvia: $keys(-bump_via) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L360) | 0108 | Unable to find techvia: $keys(-pad_via) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L428) | 0109 | Unable to find instance: $inst_name |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L433) | 0110 | Unable to find iterm: $iterm_name of $inst_name |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L438) | 0111 | Unable to find net: $net_name |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/pad.tcl#L136) | 0112 | Use of -rotation is deprecated |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1925) | 9001 | $str\nIncorrect signal assignments ([llength $errors]) found. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1721) | 9002 | Not enough bumps: available [expr 2 * ($num_signals_top_bottom + $num_signals_left_right)], required $required. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2508) | 9004 | Cannot find a terminal [get_padcell_signal_name $padcell] for ${padcell}. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L412) | 9005 | Illegal orientation \"$orient\" specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L458) | 9006 | Illegal orientation \"$orient\" specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1895) | 9007 | File $signal_assignment_file not found. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1134) | 9008 | Cannot find cell $name in the database. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5020) | 9009 | Expected 1, 2 or 4 offset values, got [llength $args]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5049) | 9010 | Expected 1, 2 or 4 inner_offset values, got [llength $args]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1683) | 9011 | Expected instance $name for padcell, $padcell not found. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2552) | 9012 | Cannot find a terminal $signal_name to associate with bondpad [$inst getName]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4671) | 9014 | Net ${signal}_$section already exists, so cannot be used in the padring. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4344) | 9015 | No cells found on $side_name side. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L768) | 9016 | Scaled core area not defined. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2038) | 9017 | Found [llength $pad_connections] top level connections to $pin_name of padcell i$padcell (inst:[$inst getName]), expecting only 1. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4696) | 9018 | No terminal $signal found on $inst_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2599) | 9019 | Cannot find shape on layer [get_footprint_pad_pin_layer] for [$inst getName]:[[$inst getMaster] getName]:[$mterm getName]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1341) | 9021 | Value of bump spacing_to_edge not specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1323) | 9022 | Cannot find padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1328) | 9023 | Signal name for padcell $padcell has not been set. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L495) | 9024 | Cannot find bondpad type in library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L566) | 9025 | No instance found for $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L506) | 9026 | Cannot find bondpad type in library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3276) | 9027 | Illegal orientation $orientation specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3296) | 9028 | Illegal orientation $orientation specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4748) | 9029 | No types specified in the library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4816) | 9030 | Unrecognized arguments to init_footprint $arglist. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L671) | 9031 | No die_area specified in the footprint specification. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L967) | 9032 | Cannot find net $signal_name for $padcell in the design. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1240) | 9033 | No value defined for pad_pin_name in the library or cell data for $type. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2727) | 9034 | No bump pitch table defined in the library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2813) | 9035 | No bump_pitch defined in library data. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2893) | 9036 | No width defined for selected bump cell $cell_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2865) | 9037 | No bump cell defined in library data. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2904) | 9038 | No bump_pin_name attribute found in the library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2916) | 9039 | No rdl_width defined in library data. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2929) | 9040 | No rdl_spacing defined in library data. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L685) | 9041 | A value for core_area must specified in the footprint specification, or in the environment variable CORE_AREA. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2095) | 9042 | Cannot find any pads on $side side. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2098) | 9043 | Pads must be defined on all sides of the die for successful extraction. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2230) | 9044 | Cannot open file $signal_map_file. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2246) | 9045 | Cannot open file $footprint_file. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2285) | 9046 | No power nets found in design. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2290) | 9047 | No ground nets found in design. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2654) | 9048 | No padcell instance found for $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1143) | 9049 | No cells defined in the library description. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4661) | 9050 | Multiple nets found on $signal in padring. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2539) | 9051 | Creating padring net: $signal_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3831) | 9052 | Creating padring net: _UNASSIGNED_$idx. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4702) | 9053 | Creating padring nets: [join $report_nets_created {, }]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L374) | 9054 | Parameter center \"$center\" missing a value for x. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L377) | 9055 | Parameter center \"$center\" missing a value for y. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L420) | 9056 | Parameter center \"$center\" missing a value for x. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L423) | 9057 | Parameter center \"$center\" missing a value for y. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L469) | 9058 | Footprint has no padcell attribute. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L473) | 9059 | No side attribute specified for padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L585) | 9060 | Cannot determine location of padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L716) | 9061 | Footprint attribute die_area has not been defined. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L727) | 9062 | Footprint attribute die_area has not been defined. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L864) | 9063 | Padcell $padcell_name not specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L870) | 9064 | No type attribute specified for padcell $padcell_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L912) | 9065 | No type attribute specified for padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1012) | 9066 | Library data has no type entry $type. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1189) | 9070 | Library does not have type $type specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1203) | 9071 | No cell $cell_name found. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1398) | 9072 | No bump attribute for padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1402) | 9073 | No row attribute specified for bump associated with padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1405) | 9074 | No col attribute specified for bump associated with padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1442) | 9075 | No bump attribute for padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1445) | 9076 | No row attribute specified for bump associated with padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1448) | 9077 | No col attribute specified for bump associated with padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2566) | 9078 | Layer [get_footprint_pin_layer] not defined in technology. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2968) | 9079 | Footprint does not have the pads_per_pitch attribute specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4428) | 9080 | Attribute $corner not specified in pad_ring ($pad_ring). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4438) | 9081 | Attribute $corner not specified in pad_ring ($pad_ring). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4563) | 9082 | Type $type not specified in the set of library types. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4567) | 9083 | Cell $cell_ref of Type $type is not specified in the list of cells in the library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4721) | 9084 | Type $type not specified in the set of library types. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4725) | 9085 | Cell $cell_ref of Type $type is not specified in the list of cells in the library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4881) | 9086 | Type $type not specified in the set of library types. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4885) | 9087 | Cell $cell_ref of Type $type is not specified in the list of cells in the library. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5293) | 9091 | No signal $signal_name defined for padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5291) | 9092 | No signal $signal_name or $try_signal defined for padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5307) | 9093 | Signal \"$signal_name\" not found in design. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5323) | 9094 | Value for -edge_name ($edge_name) not permitted, choose one of bottom, right, top or left. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5333) | 9095 | Value for -type ($type) does not match any library types ([dict keys [dict get $library types]]). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1145) | 9096 | No cell $cell_type defined in library ([dict keys [dict get $library cells]]). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1147) | 9097 | No entry found in library definition for cell $cell_type on $position side. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5336) | 9098 | No library types defined. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5434) | 9099 | Invalid orientation $orient, must be one of \"$valid\". |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5441) | 9100 | Incorrect number of arguments for location, expected an even number, got [llength $location] ($location). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5444) | 9101 | Only one of center or origin may be specified for -location ($location). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5449) | 9102 | Incorrect value specified for -location center ([dict get $location center]), $msg. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5454) | 9103 | Incorrect value specified for -location origin ([dict get $location origin]), $msg. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5457) | 9104 | Required origin or center not specified for -location ($location). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5471) | 9105 | Specification of bondpads is only allowed for wirebond padring layouts. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5480) | 9106 | Specification of bumps is only allowed for flipchip padring layouts. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5858) | 9109 | Incorrect number of arguments for add_pad - expected an even number, received [llength $args]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5876) | 9110 | Must specify -type option if -name is not specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5779) | 9111 | Unrecognized argument $arg, should be one of -pitch, -bump_pin_name, -spacing_to_edge, -cell_name, -bumps_per_tile, -rdl_layer, -rdl_width, -rdl_spacing. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5909) | 9112 | Padcell $padcell_duplicate already defined to use [dict get $padcell signal_name]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6538) | 9113 | Type specified must be flipchip or wirebond. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L604) | 9114 | No origin information specified for padcell $padcell $type $inst. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L624) | 9115 | No origin information specified for padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L787) | 9116 | Side for padcell $padcell cannot be determined. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L803) | 9117 | No orient entry for cell reference $cell_ref matching orientation $orient. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L808) | 9119 | No cell reference $cell_ref found in library data. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1167) | 9120 | Padcell $padcell does not have any location information to derive orientation. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1170) | 9121 | Padcell $padcell does not define orientation for $element. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5539) | 9122 | Cannot find an instance with name \"$inst_name\". |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5925) | 9123 | Attribute 'name' not defined for padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5955) | 9124 | Cell type $type does not exist in the set of library types. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5962) | 9125 | No type specified for padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5968) | 9126 | Only one of center or origin should be used to specify the location of padcell $padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5987) | 9127 | Cannot determine side for padcell $padcell, need to sepecify the location or the required edge for the padcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5994) | 9128 | No orientation specified for $cell_ref for side $side_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6012) | 9129 | Cannot determine cell name for $padcell_name from library element $cell_ref. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6018) | 9130 | Cell $cell_name not loaded into design. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6031) | 9131 | Orientation of padcell $padcell_name is $orient, which is different from the orientation expected for padcells on side $side_name ($side_from_orient). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6037) | 9132 | Missing orientation information for $cell_ref on side $side_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6051) | 9133 | Bondpad cell $bondpad_cell_ref not found in library definition. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6062) | 9134 | Unexpected value for orient attribute in library definition for $bondpad_cell_ref. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6066) | 9135 | Expected orientation ($expected_orient) of bondpad for padcell $padcell_name, overridden with value [dict exists $padcell bondpad orient]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6077) | 9136 | Unexpected value for orient attribute in library definition for $bondpad_cell_ref. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6081) | 9137 | Missing orientation information for $cell_ref on side $side_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5949) | 9140 | Type [dict get $padcell type] (cell ref - $expected_cell_name) does not match specified cell_name ($cell_name) for padcell $padcell). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2025) | 9141 | Signal name for padcell $padcell has not been set. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5601) | 9142 | Unexpected number of arguments for set_die_area. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5606) | 9143 | Unexpected number of arguments for set_die_area. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5626) | 9144 | Unexpected number of arguments for set_core_area. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5631) | 9145 | Unexpected number of arguments for set_core_area. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5730) | 9146 | Layer $layer_name is not a valid layer for this technology. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5707) | 9147 | The pad_inst_name value must be a format string with exactly one string substitution %s. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6005) | 9159 | Cell reference $cell_ref not found in library, setting cell_name to $cell_ref. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5716) | 9160 | The pad_pin_name value must be a format string with exactly one string substitution %s. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1060) | 9161 | Position $position not defined for $cell_ref, expecting one of [join [dict keys [dict get $library cells $cell_ref cell_name]] {, }]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2977) | 9162 | Required setting for num_pads_per_tile not found. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6110) | 9163 | Padcell [dict get $padcell inst_name] x location ([ord::dbu_to_microns [dict get $padcell cell scaled_center x]]) cannot connect to the bump $row,$col on the $side_name edge. The x location must satisfy [ord::dbu_to_microns $xMin] <= x <= [ord::dbu_to_microns $xMax]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6113) | 9164 | Padcell [dict get $padcell inst_name] y location ([ord::dbu_to_microns [dict get $padcell cell scaled_center y]]) cannot connect to the bump $row,$col on the $side_name edge. The y location must satisfy [ord::dbu_to_microns $yMin] <= y <= [ord::dbu_to_microns $yMax]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6165) | 9165 | Attribute 'name' not defined for cell $cell_inst. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6178) | 9166 | Type [dict get $cell_inst type] (cell ref - $expected_cell_name) does not match specified cell_name ($cell_name) for cell $name). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6184) | 9167 | Cell type $type does not exist in the set of library types. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6191) | 9168 | No type specified for cell $name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6197) | 9169 | Only one of center or origin should be used to specify the location of cell $name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6207) | 9170 | Cannot determine library cell name for cell $name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6213) | 9171 | Cell $cell_name not loaded into design. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6228) | 9173 | No orientation information available for $name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6312) | 9174 | Unexpected keyword in cell name specification, $msg. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6323) | 9175 | Unexpected keyword in orient specification, $orient_by_side. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6343) | 9176 | Cannot find $cell_name in the database. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6347) | 9177 | Pin $pin_name does not exist on cell $cell_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6363) | 9178 | Incorrect number of arguments for add_pad, expected an even number, received [llength $args]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6371) | 9179 | Must specify -name option for add_libcell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6410) | 9180 | Library cell reference missing name attribute. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6426) | 9181 | Library cell reference $cell_ref_name missing type attribute. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6437) | 9182 | Type of $cell_ref_name ($type) clashes with existing setting for type ([dict get $library types $type]). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6478) | 9183 | No specification found for which cell names to use on each side for padcell $cell_ref_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6492) | 9184 | No specification found for the orientation of cells on each side. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6512) | 9185 | No specification of the name of the external pin on cell_ref $cell_ref_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L365) | 9187 | Signal $break_signal not defined in the list of signals to connect by abutment. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6616) | 9188 | Invalid placement status $placement_status, must be one of either PLACED or FIRM. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6625) | 9189 | Cell $cell_name not loaded into design. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6632) | 9190 | -inst_name is a required argument to the place_cell command. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6640) | 9191 | Invalid orientation $orient specified, must be one of [join $valid_orientation {, }]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6643) | 9192 | No orientation specified for $inst_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6650) | 9193 | Origin is $origin, but must be a list of 2 numbers. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6653) | 9194 | Invalid value specified for x value, [lindex $origin 0], $msg. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6656) | 9195 | Invalid value specified for y value, [lindex $origin 1], $msg. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6659) | 9196 | No origin specified for $inst_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6666) | 9197 | Instance $inst_name not in the design, -cell must be specified to create a new instance. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6671) | 9198 | Instance $inst_name expected to be $cell_name, but is actually [[$inst getMaster] getName]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6677) | 9199 | Cannot create instance $inst_name of $cell_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5901) | 9200 | Unrecognized argument $arg, should be one of -name, -signal, -edge, -type, -cell, -location, -bump, -bondpad, -inst_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6389) | 9201 | Unrecognized argument $arg, should be one of -name, -type, -cell_name, -orient, -pad_pin_name, -break_signals, -physical_only. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6397) | 9202 | Padcell $padcell_duplicate already defined to use [dict get $padcell signal_name]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L358) | 9203 | No cell type $breaker_cell_type defined. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L361) | 9204 | No cell [dict get $library types $breaker_cell_type] defined. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5685) | 9205 | Incorrect number of values specified for offsets ([llength $value]), expected 1, 2 or 4. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4779) | 9207 | Required type of cell ($required_type) has no libcell definition. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L205) | 9208 | Type option already set to [dict get $args -type], option $flag cannot be used to reset the type. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5789) | 9209 | The number of padcells within a pad pitch ($num_pads_per_tile) must be a number between 1 and 5. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5794) | 9210 | The number of padcells within a pad pitch (pitch $pitch: num_padcells: $value) must be a number between 1 and 5. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5807) | 9211 | No RDL layer specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5818) | 9212 | Width set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_width]), is less than the minimum width of the layer in this technology ([ord::dbu_to_microns $min_width]). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5821) | 9213 | Width set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_width]), is greater than the maximum width of the layer in this technology ([ord::dbu_to_microns $max_width]). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5833) | 9214 | Spacing set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_spacing]), is less than the required spacing for the layer in this technology ([ord::dbu_to_microns $spacing]). |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5840) | 9215 | The number of pads within a bump pitch has not been specified. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5880) | 9216 | A padcell with the name $padcell_name already exists. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2381) | 9217 | Attribute $attribute $value for padcell $name has already been used for padcell [dict get $checks $attribute $value]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L51) | 9218 | Unrecognized arguments ([lindex $args 0]) specified for set_bump_options. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L124) | 9219 | Unrecognized arguments ([lindex $args 0]) specified for set_padring_options. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L195) | 9220 | Unrecognized arguments ([lindex $args 0]) specified for define_pad_cell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L235) | 9221 | Unrecognized arguments ([lindex $args 0]) specified for add_pad. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L250) | 9222 | Unrecognized arguments ([lindex $args 0]) specified for initialize_padring. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2353) | 9223 | Design data must be loaded before this command. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L231) | 9224 | Design must be loaded before calling add_pad. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L191) | 9225 | Library must be loaded before calling define_pad_cell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L116) | 9226 | Design must be loaded before calling set_padring_options. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L243) | 9227 | Design must be loaded before calling initialize_padring. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L6604) | 9228 | Design must be loaded before calling place_cell. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5421) | 9229 | The value for row is $row, but must be in the range 1 - $num_bumps_y. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5424) | 9230 | The value for col is $col, but must be in the range 1 - $num_bumps_x. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L60) | 9231 | Design must be loaded before calling set_bump. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L83) | 9232 | The -power -ground and -net options are mutualy exclusive for the set_bump command. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L72) | 9233 | Required option -row missing for set_bump. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L75) | 9234 | Required option -col missing for set_bump. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3736) | 9235 | Net $net_name specified as a $type net, but has alreaqdy been defined as a [dict get $bumps nets $net_name] net. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3863) | 9236 | Bump $row $col is not assigned to power or ground. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L68) | 9237 | Unrecognized arguments ([lindex $args 0]) specified for set_bump. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3709) | 9238 | Trying to set bump at ($row $col) to be $net_name, but it has already been set to [dict get $bumps $row $col net]. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5351) | 9239 | expecting a 2 element list in the form \"number number\". |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5344) | 9240 | Invalid coordinate specified $msg. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5375) | 9241 | expecting a 2 element list in the form \"number number\". |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5379) | 9242 | Invalid array_size specified $msg. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5408) | 9243 | expecting a 4 element list in the form \"row <integer> col <integer>\". |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5411) | 9244 | row value ([dict get $rowcol row]), not recognized as an integer. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5414) | 9245 | col value ([dict get $rowcol col]), not recognized as an integer. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5777) | 9246 | The use of a cover DEF is deprecated, as all RDL routes are writen to the database |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4221) | 9247 | Cannot fit IO pads between the following anchor cells : $anchor_cell_a, $anchor_cell_b. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4272) | 9248 | The max_spacing constraint cannot be met for cell $padcell ($padcellRef), $max_spacing_ref needs to be adjacent to $padcellRef. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4240) | 9249 | The max_spacing constraint cannot be met for cell $anchor_cell_a ($padcellRef), and $anchor_cell_b ($padcellRefB), because adjacent cell displacement is larger than the constraint. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L4033) | 9250 | No center information specified for $inst_name. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L1123) | 9251 | Cannot find cell $name in the database. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2642) | 9252 | Bondpad cell [[$inst getMaster] getName], does not have the specified pin name ($pin_name) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3814) | 9253 | Bump cell [[$inst getMaster] getName], does not have the specified pin name ($pin_name) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5081) | 9254 | Unfilled gaps in the padring on $side side |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5083) | 9255 | [ord::dbu_to_microns [lindex $gap 0]] -> [ord::dbu_to_microns [lindex $gap 1]] |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5086) | 9256 | Padcell ring cannot be filled |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L2621) | 9257 | Cannot create bondpad instance bp_${signal_name} |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5749) | 9258 | Routing style must be 45, 90 or under. Illegal value \"$value\" specified |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L5739) | 9259 | Via $via does not exist. |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3465) | 9260 | No via has been defined to connect from padcells to rdl |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3500) | 9261 | No via has been defined to connect from rdl to bump |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3441) | 9262 | RDL path trace for $padcell (bump: $row, $col) is further from the core than the padcell pad pin |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3445) | 9263 | RDL path has an odd number of cor-ordinates |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3587) | 9264 | Malformed point ([lindex $points 0]) for padcell $padcell (points: $points) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3625) | 9265 | Malformed point ($p1) for padcell $padcell (points: $points) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3627) | 9266 | Malformed point ($p2) for padcell $padcell (points: $points) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3656) | 9267 | Malformed point ($p1) for padcell $padcell (points: $points) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3658) | 9268 | Malformed point ($p2) for padcell $padcell (points: $points) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3665) | 9269 | Malformed point ($prev) for padcell $padcell (points: $points) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3667) | 9270 | Malformed point ($point) for padcell $padcell (points: $points) |
| [PAD](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/ICeWall.tcl#L3677) | 9271 | Malformed point ([lindex $points end]) for padcell $padcell (points: $points) |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Coarsener.cpp#L155) | 0001 | Hierarchical coarsening time {} seconds |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L252) | 0002 | Number of partitions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L253) | 0003 | UBfactor = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L254) | 0004 | Seed = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L255) | 0005 | Vertex dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L256) | 0006 | Hyperedge dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L257) | 0007 | Placement dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L258) | 0008 | Hypergraph file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L259) | 0009 | Solution file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L260) | 0010 | Global net threshold = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L262) | 0011 | Fixed file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L265) | 0012 | Community file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L268) | 0013 | Group file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L271) | 0014 | Placement file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/PartitionMgr.cpp#L755) | 0015 | Property 'partition_id' not found for inst {}. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L370) | 0016 | UBfactor = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L371) | 0017 | Seed = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L372) | 0018 | Vertex dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L373) | 0019 | Hyperedge dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L374) | 0020 | Placement dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L375) | 0021 | Timing aware flag = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/PartitionMgr.cpp#L844) | 0022 | Unable to open file {}. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L377) | 0023 | Global net threshold = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L378) | 0024 | Top {} critical timing paths are extracted. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L379) | 0025 | Fence aware flag = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L381) | 0026 | fence_lx = {}, fence_ly = {}, fence_ux = {}, fence_uy = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L390) | 0027 | Fixed file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L393) | 0028 | Community file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L396) | 0029 | Group file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L399) | 0030 | Solution file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L523) | 0031 | Number of partitions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L524) | 0032 | UBfactor = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L525) | 0033 | Seed = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L526) | 0034 | Vertex dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L527) | 0035 | Hyperedge dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L528) | 0036 | Placement dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L529) | 0037 | Hypergraph file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L530) | 0038 | Solution file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L532) | 0039 | Fixed file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L535) | 0040 | Community file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L538) | 0041 | Group file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L541) | 0042 | Placement file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L579) | 0043 | hyperedge weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L591) | 0044 | vertex weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L608) | 0045 | placement weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L621) | 0046 | net_timing_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L622) | 0047 | path_timing_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L623) | 0048 | path_snaking_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L624) | 0049 | timing_exp_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L625) | 0050 | extra_delay : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/partitionmgr.tcl#L952) | 0051 | Missing mandatory argument -read_file |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L715) | 0052 | UBfactor = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L716) | 0053 | Seed = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L717) | 0054 | Vertex dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L718) | 0055 | Hyperedge dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L719) | 0056 | Placement dimensions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L720) | 0057 | Guardband flag = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L721) | 0058 | Timing aware flag = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L722) | 0059 | Global net threshold = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L723) | 0060 | Top {} critical timing paths are extracted. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L724) | 0061 | Fence aware flag = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L726) | 0062 | fence_lx = {}, fence_ly = {}, fence_ux = {}, fence_uy = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L735) | 0063 | Fixed file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L738) | 0064 | Community file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L741) | 0065 | Group file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L744) | 0066 | Hypergraph file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L747) | 0067 | Hypergraph_int_weight_file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L751) | 0068 | Solution file = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L776) | 0069 | hyperedge weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L788) | 0070 | vertex weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/PartitionMgr.cpp#L854) | 0071 | Unable to convert line \"{}\" to an integer in file: {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/PartitionMgr.cpp#L822) | 0072 | Unable to open file {}. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/PartitionMgr.cpp#L831) | 0073 | Unable to find instance {}. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/PartitionMgr.cpp#L865) | 0074 | Instances in partitioning ({}) does not match instances in netlist ({}). |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L814) | 0075 | timing_exp_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L815) | 0076 | extra_delay : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1696) | 0077 | hyperedge weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1708) | 0078 | vertex weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1725) | 0079 | placement weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1730) | 0080 | net_timing_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1731) | 0081 | path_timing_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1732) | 0082 | path_snaking_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1733) | 0083 | timing_exp_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1735) | 0084 | coarsen order : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1736) | 0085 | thr_coarsen_hyperedge_size_skip : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1740) | 0086 | thr_coarsen_vertices : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1741) | 0087 | thr_coarsen_hyperedges : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1743) | 0088 | coarsening_ratio : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1744) | 0089 | max_coarsen_iters : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1745) | 0090 | adj_diff_ratio : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1746) | 0091 | min_num_vertcies_each_part : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1749) | 0092 | num_initial_solutions : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1750) | 0093 | num_best_initial_solutions : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1753) | 0094 | refine_iters : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1754) | 0095 | max_moves (FM or greedy refinement) : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1756) | 0096 | early_stop_ratio : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1757) | 0097 | total_corking_passes : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1758) | 0098 | v_cycle_flag : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1759) | 0099 | max_num_vcycle : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1760) | 0100 | num_coarsen_solutions : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1761) | 0101 | num_vertices_threshold_ilp : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L369) | 0102 | Number of partitions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L376) | 0103 | Guardband flag = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L714) | 0104 | Number of partitions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L805) | 0105 | placement weight factor : [ {} ] |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L811) | 0106 | net_timing_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L812) | 0107 | path_timing_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L813) | 0108 | path_snaking_factor : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1933) | 0109 | The runtime of multi-level partitioner : {} seconds |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L464) | 0110 | Updated solution file name = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L113) | 0111 | This no timing-critical paths when calling GetPathTimingScore() |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L130) | 0112 | This no timing-critical paths when calling CalculatePathsCost() |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L178) | 0113 | This no timing-critical paths when calling GetPathsCost() |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L195) | 0114 | This no timing-critical paths when calling GetTimingCuts() |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/ILPRefine.cpp#L134) | 0115 | ILP-based partitioning cannot find a valid solution. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Partitioner.cpp#L276) | 0116 | ilp_accelerator_factor = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Partitioner.cpp#L278) | 0117 | No hyperedges will be used !!! |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Refiner.cpp#L119) | 0118 | Exit Refinement. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L278) | 0119 | Reset the timing_aware_flag to false. Timing-driven mode is not supported |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L559) | 0120 | Reset the timing_aware_flag to false. Timing-driven mode is not supported |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L571) | 0121 | no hyperedge weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L585) | 0124 | No vertex weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L598) | 0125 | No placement weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L768) | 0126 | No hyperedge weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L782) | 0127 | No vertex weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L795) | 0128 | No placement weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1034) | 0129 | Reset the fixed attributes to NONE. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1052) | 0130 | Reset the community attributes to NONE. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1139) | 0132 | Reset the placement attributes to NONE. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1292) | 0133 | Cannot open the fixed instance file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1319) | 0134 | Cannot open the community file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1346) | 0135 | Cannot open the group file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1484) | 0136 | Timing driven partitioning is disabled |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1669) | 0137 | {} unconstrained hyperedges ! |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1674) | 0138 | Reset the slack of all unconstrained hyperedges to {} seconds |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1688) | 0139 | No hyperedge weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1715) | 0140 | No placement weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1702) | 0141 | No vertex weighting is specified. Use default value of 1. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L290) | 0142 | This no timing-critical paths when calling GetTimingCuts() |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L249) | 0143 | Total number of timing paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L251) | 0144 | Total number of timing-critical paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L255) | 0145 | Total number of timing-noncritical paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L259) | 0146 | The worst number of cuts on timing-critical paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L263) | 0147 | The average number of cuts on timing-critical paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L267) | 0148 | Total number of timing-noncritical to timing critical paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L272) | 0149 | The worst number of cuts on timing-non2critical paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Evaluator.cpp#L276) | 0150 | The average number of cuts on timing-non2critical paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L120) | 0151 | Finish Candidate Solutions Generation |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L129) | 0152 | Finish Cut-Overlay Clustering and Optimal Partitioning |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L141) | 0153 | Finish Vcycle Refinement |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L218) | 0154 | [V-cycle Refinement] num_cycles = {}, cutcost = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L442) | 0155 | Number of chosen best initial solutions = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L447) | 0156 | Best initial cutcost {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L623) | 0157 | Cut-Overlay Clustering : num_vertices = {}, num_hyperedges = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Multilevel.cpp#L650) | 0158 | Statistics of cut-overlay solution: |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Partitioner.cpp#L62) | 0159 | Set ILP accelerator factor to {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Partitioner.cpp#L69) | 0160 | Reset ILP accelerator factor to {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Partitioner.cpp#L268) | 0161 | ilp_accelerator_factor = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Partitioner.cpp#L270) | 0162 | Reduce the number of hyperedges from {} to {}. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Refiner.cpp#L93) | 0163 | Set the max_move to {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Refiner.cpp#L99) | 0164 | Set the refiner_iter to {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Refiner.cpp#L107) | 0165 | Reset the max_move to {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/Refiner.cpp#L108) | 0166 | Reset the refiner_iters to {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L235) | 0167 | Partitioning parameters**** |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L333) | 0168 | [INFO] Partitioning parameters**** |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L504) | 0169 | Partitioning parameters**** |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L676) | 0170 | Partitioning parameters**** |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1157) | 0171 | Hypergraph Information** |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1158) | 0172 | Vertices = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1160) | 0173 | Hyperedges = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1468) | 0174 | Netlist Information** |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1469) | 0175 | Vertices = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1471) | 0176 | Hyperedges = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1473) | 0177 | Number of timing paths = {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1616) | 0178 | maximum_clock_period : {} second |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1619) | 0179 | normalized extra delay : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1630) | 0180 | We normalized the slack of each path based on maximum clock period |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1640) | 0181 | We normalized the slack of each net based on maximum clock period |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/partitionmgr.tcl#L110) | 0924 | Missing mandatory argument -hypergraph_file. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/partitionmgr.tcl#L332) | 0925 | Missing mandatory argument -hypergraph_file. |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L944) | 2500 | Can not open the input hypergraph file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1027) | 2501 | Can not open the fixed file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1044) | 2502 | Can not open the community file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1061) | 2503 | Can not open the group file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1084) | 2504 | Can not open the placement file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L547) | 2511 | Can not open the solution file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L850) | 2514 | Can not open the solution file : {} |
| [PAR](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/par/src/TritonPart.cpp#L1442) | 2677 | There is no vertices and hyperedges |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid.cpp#L133) | 0001 | Inserting grid: {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/domain.cpp#L260) | 0100 | Unable to find {} net for {} domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/domain.cpp#L282) | 0101 | Using {} as power net for {} domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/domain.cpp#L291) | 0102 | Using {} as ground net for {} domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/domain.cpp#L84) | 0103 | {} region must have a shape. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/domain.cpp#L87) | 0104 | {} region contains {} shapes, but only one is supported. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/rings.cpp#L123) | 0105 | Unable to determine location of pad offset, using die boundary instead. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid_component.cpp#L364) | 0106 | Width ({:.4f} um) specified for layer {} is less than minimum width ({:.4f} um). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid_component.cpp#L376) | 0107 | Width ({:.4f} um) specified for layer {} is greater than maximum width ({:.4f} um). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid_component.cpp#L446) | 0108 | Spacing ({:.4f} um) specified for layer {} is less than minimum spacing ({:.4f} um). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/straps.cpp#L473) | 0109 | Unable to determine width of followpin straps from standard cells. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/via.cpp#L938) | 0110 | No via inserted between {} and {} at {} on {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L572) | 0113 | The grid $grid_name has not been defined. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid_component.cpp#L424) | 0114 | Width ({:.4f} um) specified for layer {} in not a valid width, must be {}. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6608) | 0174 | Net $net_name has no global connections defined. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/straps.cpp#L102) | 0175 | Pitch {:.4f} is too small for, must be atleast {:.4f} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/straps.cpp#L2004) | 0178 | Remaining channel {} on {} for nets: {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/straps.cpp#L2013) | 0179 | Unable to repair all channels. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/rings.cpp#L63) | 0180 | Ring cannot be build with layers following the same direction: {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/domain.cpp#L267) | 0181 | Found multiple possible nets for {} net for {} domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/PdnGen.cc#L455) | 0182 | Instance {} already belongs to another grid \"{}\" and therefore cannot belong to \"{}\". |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/PdnGen.cc#L285) | 0183 | Replacing existing core voltage domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/PdnGen.cc#L306) | 0184 | Cannot have region voltage domain with the same name already exists: {} |
| [PDN](\"{}\) | 0185 | Insufficient width ({} um) to add straps on layer {} in grid |
| ["with](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/straps.cpp#L127) | total | width {} um and offset {} um. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid.cpp#L115) | 0186 | Connect between layers {} and {} already exists in \"{}\". |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/straps.cpp#L82) | 0187 | Unable to place strap on {} with unknown routing direction. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid.cpp#L1527) | 0188 | Existing grid does not support adding {}. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/PdnGen.cc#L799) | 0189 | Supply pin {} of instance {} is not connected to any net. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/straps.cpp#L347) | 0190 | Unable to determine the pitch of the rows. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/techlayer.cpp#L147) | 0191 | {} of {:.4f} does not fit the manufacturing grid of {:.4f}. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid.cpp#L602) | 0192 | There are multiple ({}) followpin definitions in {}, but no connect statements between them. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid.cpp#L610) | 0193 | There are only ({}) followpin connect statements when {} is/are required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid.cpp#L634) | 0194 | Connect statements for followpins overlap between layers: {} -> {} and {} -> {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/via.cpp#L2827) | 0195 | Removing {} via(s) between {} and {} at ({:.4f} um, {:.4f} um) for {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/PdnGen.cc#L339) | 0196 | {} is already defined. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/power_cells.cpp#L154) | 0197 | Unrecognized network type: {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/power_cells.cpp#L124) | 0198 | {} requires the power cell to have an acknowledge pin. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L153) | 0199 | Net $switched_power_net_name already exists in the design, but is of signal type [$switched_power getSigType]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/power_cells.cpp#L247) | 0220 | Unable to find a strap to connect power switched to. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/power_cells.cpp#L300) | 0221 | Instance {} should be {}, but is {}. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/power_cells.cpp#L236) | 0222 | Power switch insertion has already run. To reset use -ripup option. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/power_cells.cpp#L328) | 0223 | Unable to insert power switch ({}) at ({:.4f}, {:.4f}), due to lack of available rows. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/grid_component.cpp#L472) | 0224 | {} is not a net in {}. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L340) | 0225 | Unable to find net $net_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/via_repair.cpp#L130) | 0226 | {} contains block vias to be removed, which is not supported. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/via.cpp#L860) | 0227 | Removing between {} and {} at ({:.4f} um, {:.4f} um) for {} |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/PdnGen.cc#L682) | 0228 | Unable to open \"{}\" to write. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/PdnGen.cc#L302) | 0229 | Region must be specified. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L463) | 0230 | Unable to find net $net_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L99) | 1001 | The -power argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L103) | 1002 | Unable to find power net: $keys(-power) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L108) | 1003 | The -ground argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L112) | 1004 | Unable to find ground net: $keys(-ground) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L121) | 1005 | Unable to find region: $keys(-region) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L136) | 1006 | Unable to find secondary power net: $snet |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L290) | 1007 | The -layer argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L295) | 1008 | The -width argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L301) | 1009 | The -pitch argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L354) | 1010 | Options -extend_to_core_ring and -extend_to_boundary are mutually exclusive. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L416) | 1011 | The -layers argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L420) | 1012 | Expecting a list of 2 elements for -layers option of add_pdn_ring command, found [llength $keys(-layers)]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L424) | 1013 | The -widths argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L430) | 1014 | The -spacings argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L436) | 1015 | Only one of -pad_offsets or -core_offsets can be specified. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L438) | 1016 | One of -pad_offsets or -core_offsets must be specified. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L450) | 1017 | Only one of -pad_offsets or -core_offsets can be specified. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L554) | 1019 | The -layers argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L556) | 1020 | The -layers must contain two layers. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L592) | 1021 | Unable to find via: $via |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L705) | 1022 | Design must be loaded before calling $args. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L719) | 1023 | Unable to find $name layer. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L728) | 1024 | $key has been deprecated$use |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L749) | 1025 | -name is required |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L840) | 1026 | Options -grid_over_pg_pins and -grid_over_boundary are mutually exclusive. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L856) | 1027 | Options -instances, -cells, and -default are mutually exclusive. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L858) | 1028 | Either -instances, -cells, or -default must be specified. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L867) | 1029 | -name is required |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L909) | 1030 | Unable to find instance: $inst_pattern |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L960) | 1032 | Unable to find $name domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L1007) | 1033 | Argument $arg must consist of 1 or 2 entries. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L1020) | 1034 | Argument $arg must consist of 1, 2 or 4 entries. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L1039) | 1035 | Unknown -starts_with option: $value |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L1062) | 1036 | Invalid orientation $orient specified, must be one of [join $valid_orientations {, }]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L49) | 1037 | -reset flag is mutually exclusive to all other flags |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L56) | 1038 | -ripup flag is mutually exclusive to all other flags |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L63) | 1039 | -report_only flag is mutually exclusive to all other flags |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L1357) | 1040 | File $config does not exist. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L1360) | 1041 | File $config is empty. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L160) | 1042 | Core voltage domain will be named \"Core\". |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L753) | 1043 | Grid named \"$keys(-name)\" already defined. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L870) | 1044 | Grid named \"$keys(-name)\" already defined. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L783) | 1045 | -power_control must be specified with -power_switch_cell |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L224) | 1046 | Unable to find power switch cell master: $keys(-name) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L1046) | 1047 | Unable to find $term on $master |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L779) | 1048 | Switched power cell $keys(-power_switch_cell) is not defined. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L787) | 1049 | Unable to find power control net: $keys(-power_control) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L220) | 1183 | The -name argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L229) | 1184 | The -control argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L240) | 1186 | The -power_switchable argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L246) | 1187 | The -power argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L252) | 1188 | The -ground argument is required. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L660) | 1190 | Unable to find net: $keys(-net) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L650) | 1191 | Cannot use both -net and -all arguments. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pdn/src/pdn.tcl#L653) | 1192 | Must use either -net or -all arguments. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3142) | 9002 | No shapes on layer $l1 for $net. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3146) | 9003 | No shapes on layer $l2 for $net. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3156) | 9004 | Unexpected number of points in connection shape ($l1,$l2 $net [llength $points]). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5360) | 9006 | Unexpected number of points in stripe of $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4928) | 9008 | Design name is $design_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4975) | 9009 | Reading technology data. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6783) | 9010 | Inserting macro grid for [llength [dict keys $instances]] macros. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6819) | 9011 | ****** INFO ****** |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6829) | 9012 | **** END INFO **** |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6833) | 9013 | Inserting stdcell grid - [dict get $specification name]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6835) | 9014 | Inserting stdcell grid. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6864) | 9015 | Writing to database. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6884) | 9016 | Power Delivery Network Generator: Generating PDN\n config: $config |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4230) | 9017 | No stdcell grid specification found - no rails can be inserted. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6816) | 9018 | No macro grid specifications found - no straps added for macros. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1468) | 9019 | Cannot find layer $layer_name in loaded technology. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1640) | 9020 | Failed to read CUTCLASS property '$line'. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1705) | 9021 | Failed to read ENCLOSURE property '$line'. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2918) | 9022 | Cannot find lower metal layer $layer1. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2919) | 9023 | Cannot find upper metal layer $layer2. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2926) | 9024 | Missing logical viarule [dict get $intersection rule].\nAvailable logical viarules [dict keys $logical_viarules]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3312) | 9025 | Unexpected row orientation $orient for row [$row getName]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3458) | 9026 | Invalid direction \"[get_dir $layer]\" for metal layer ${layer}. Should be either \"hor\" or \"ver\". |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4738) | 9027 | Illegal orientation $orientation specified. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6894) | 9028 | File $PDN_cfg is empty. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4881) | 9029 | Illegal number of elements defined for ::halo \"$::halo\" (1, 2 or 4 allowed). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5020) | 9030 | Layer specified for stdcell rails '$layer' not in list of layers. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5640) | 9032 | Generating blockages for TritonRoute. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1376) | 9033 | Unknown direction for layer $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6786) | 9034 | - grid [dict get $grid_data name] for instance $instance |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5720) | 9035 | No track information found for layer $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2752) | 9036 | Attempt to add illegal via at : ([ord::dbu_to_microns [lindex $via_location 0]] [ord::dbu_to_microns [lindex $via_location 1]]), via will not be added. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4782) | 9037 | Pin $term_name of instance [$inst getName] is not connected to any net. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2725) | 9038 | Illegal via: number of cuts ($num_cuts), does not meet minimum cut rule ($min_cut_rule) for $lower_layer to $cut_class with width [ord::dbu_to_microns $lower_width]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2746) | 9039 | Illegal via: number of cuts ($num_cuts), does not meet minimum cut rule ($min_cut_rule) for $upper_layer to $cut_class with width [ord::dbu_to_microns $upper_width]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3175) | 9040 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full height of $layer1 ([ord::dbu_to_microns [get_grid_wire_width $layer1]]) is not covered by the overlap. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3181) | 9041 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full width of $layer1 ([ord::dbu_to_microns [get_grid_wire_width $layer1]]) is not covered by the overlap. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3189) | 9042 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full height of $layer2 ([ord::dbu_to_microns [get_grid_wire_width $layer2]]) is not covered by the overlap. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3195) | 9043 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full width of $layer2 ([ord::dbu_to_microns [get_grid_wire_width $layer2]]) is not covered by the overlap. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3076) | 9044 | No width information found for $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3097) | 9045 | No pitch information found for $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3979) | 9048 | Need to define pwr_pads and gnd_pads in config file to use pad_offset option. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5966) | 9051 | Infinite loop detected trying to round to grid. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3038) | 9052 | Unable to get channel_spacing setting for layer $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5085) | 9055 | Cannot find pin $pin_name on inst [$inst getName]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5089) | 9056 | Cannot find master pin $pin_name for cell [[$inst getMaster] getName]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6890) | 9062 | File $PDN_cfg does not exist. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2905) | 9063 | Via $via_name specified in the grid specification does not exist in this technology. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4039) | 9064 | No power/ground pads found on bottom edge. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4042) | 9065 | No power/ground pads found on right edge. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4045) | 9066 | No power/ground pads found on top edge. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4048) | 9067 | No power/ground pads found on left edge. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4051) | 9068 | Cannot place core rings without pwr/gnd pads on each side. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4599) | 9069 | Cannot find via $via_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4644) | 9070 | Cannot find net $net_name in the design. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4650) | 9071 | Cannot create terminal for net $net_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L86) | 9072 | Design must be loaded before calling pdngen commands. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L101) | 9074 | Invalid orientation $orient specified, must be one of [join $valid_orientations {, }]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L114) | 9075 | Layer $actual_layer_name not found in loaded technology data. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L117) | 9076 | Layer $layer_name not found in loaded technology data. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L132) | 9077 | Width ($width) specified for layer $layer_name is less than minimum width ([ord::dbu_to_microns $minWidth]). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L135) | 9078 | Width ($width) specified for layer $layer_name is greater than maximum width ([ord::dbu_to_microns $maxWidth]). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L147) | 9079 | Spacing ($spacing) specified for layer $layer_name is less than minimum spacing ([ord::dbu_to_microns $minSpacing)]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L154) | 9081 | Expected an even number of elements in the list for -rails option, got [llength $rails_spec]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L173) | 9083 | Expected an even number of elements in the list for straps specification, got [llength $straps_spec]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L180) | 9084 | Missing width specification for strap on layer $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L194) | 9085 | Pitch [dict get $straps_spec $layer_name pitch] specified for layer $layer_name is less than 2 x (width + spacing) (width=[ord::dbu_to_microns $width], spacing=[ord::dbu_to_microns $spacing]). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L197) | 9086 | No pitch specified for strap on layer $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L206) | 9087 | Connect statement must consist of at least 2 entries. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L545) | 9088 | Unrecognized argument $arg, should be one of -name, -orient, -instances -cells -pins -starts_with. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1191) | 9090 | The orient attribute cannot be used with stdcell grids. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L273) | 9095 | Value specified for -starts_with option ($value), must be POWER or GROUND. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L229) | 9109 | Expected an even number of elements in the list for core_ring specification, got [llength $core_ring_spec]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L284) | 9110 | Voltage domain $domain has not been specified, use set_voltage_domain to create this voltage domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L296) | 9111 | Instance $instance does not exist in the design. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L306) | 9112 | Cell $cell not loaded into the database. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L702) | 9114 | Unexpected value ($value), must be either POWER or GROUND. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L907) | 9115 | Unexpected number of values for -widths, $msg. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L920) | 9116 | Unexpected number of values for -spacings, $msg. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L933) | 9117 | Unexpected number of values for -core_offsets, $msg. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L946) | 9118 | Unexpected number of values for -pad_offsets, $msg. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L973) | 9119 | Via $via_name specified in the grid specification does not exist in this technology. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L237) | 9121 | Missing width specification for strap on layer $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L736) | 9124 | Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L959) | 9125 | Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1004) | 9126 | Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L317) | 9127 | No region $region_name found in the design for voltage_domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L358) | 9128 | Net $power_net_name already exists in the design, but is of signal type [$net getSigType]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L395) | 9129 | Net $ground_net_name already exists in the design, but is of signal type [$net getSigType]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L442) | 9130 | Unrecognized argument $arg, should be one of -name, -power, -ground -region. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L452) | 9138 | Unexpected value for direction ($direction), should be horizontal or vertical. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L257) | 9139 | No direction defined for layers [dict keys $core_ring_spec]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L263) | 9140 | Layers [dict keys $core_ring_spec] are both $direction, missing layer in direction $other_direction. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L265) | 9141 | Unexpected number of directions found for layers [dict keys $core_ring_spec], ([dict keys $layer_directions]). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L253) | 9146 | Must specify a pad_offset or core_offset for rings. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1139) | 9147 | No definition of power padcells provided, required when using pad_offset. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1142) | 9148 | No definition of ground padcells provided, required when using pad_offset. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L831) | 9149 | Power net $net_name not found. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L845) | 9150 | Cannot find cells ([join $find_cells {, }]) in voltage domain $voltage_domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L864) | 9151 | Ground net $net_name not found. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L878) | 9152 | Cannot find cells ([join $find_cells {, }]) in voltage domain $voltage_domain. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1151) | 9153 | Core power padcell ($cell) not found in the database. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1154) | 9154 | Cannot find pin ($pin_name) on core power padcell ($cell). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1164) | 9155 | Core ground padcell ($cell) not found in the database. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1167) | 9156 | Cannot find pin ($pin_name) on core ground padcell ($cell). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L753) | 9158 | No voltage domains defined for grid. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L764) | 9159 | Voltage domains $domain_name has not been defined. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5687) | 9160 | Cannot find layer $layer_name. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L496) | 9164 | Problem with halo specification, $msg. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1093) | 9165 | Conflict found, instance $inst_name is part of two grid definitions ($grid_name, [dict get $instances $inst_name grid]). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L1277) | 9166 | Instance $inst of cell [dict get $instance macro] is not associated with any grid. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L2994) | 9168 | Layer $layer_name does not exist |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6132) | 9169 | Cannot fit additional $net horizontal strap in channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin]) - ([ord::dbu_to_microns $xMax], [ord::dbu_to_microns $yMax]) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6152) | 9170 | Cannot fit additional $net vertical strap in channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin]) - ([ord::dbu_to_microns $xMax], [ord::dbu_to_microns $yMax]) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6160) | 9171 | Channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) too narrow. Channel on layer $layer_name must be at least [ord::dbu_to_microns [expr round(2.0 * $width + $channel_spacing)]] wide. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L6169) | 9172 | Channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) too narrow. Channel on layer $layer_name must be at least [ord::dbu_to_microns [expr round(2.0 * $width + $channel_spacing)]] wide. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L377) | 9176 | Net $secondary_power already exists in the design, but is of signal type [$net getSigType]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L4788) | 9177 | Cannot find pin $term_name on instance [$inst getName] ([[$inst getMaster] getName]). |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L486) | 9178 | Problem with max_columns specification, $msg. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L476) | 9179 | Problem with max_rows specification, $msg. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L342) | 9180 | Net $switched_power_net_name already exists in the design, but is of signal type [$net getSigType]. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L326) | 9181 | Switch cell $switch_cell not loaded into the database. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L417) | 9190 | Unrecognized argument $arg, should be one of -name, -control, -acknowledge, -power, -ground. |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3804) | 9191 | No power control signal is defined for a grid that includes power switches |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3809) | 9192 | Cannot find power control signal [dict get $power_switch control_signal] |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3822) | 9193 | Cannot find instance term $control_pin for [$inst getName] of cell [[$inst getMaster] getName] |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L505) | 9194 | Net $value does not exist in the design |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L517) | 9195 | Option -power_control_network must be set to STAR or DAISY |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3859) | 9196 | Invalid value specified for power control network type |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L3850) | 9197 | Cannot find pin $ack_pin_name on power switch [$inst getName] ($cell_name) |
| [PDN](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/pad/src/PdnGen.tcl#L5654) | 9248 | Instance $instance is not associated with any grid |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1214) | 0001 | Number of slots {} |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1215) | 0002 | Number of I/O {} |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1217) | 0003 | Number of I/O w/sink {} |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1219) | 0004 | Number of I/O w/o sink {} |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1220) | 0005 | Slots per section {} |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1221) | 0006 | Slots increase factor {:.1} |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1776) | 0007 | Random pin placement. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1161) | 0008 | Successfully assigned pins to sections. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1164) | 0009 | Unsuccessfully assigned pins to sections ({} out of {}). |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1231) | 0010 | Tentative {} to set up sections. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1852) | 0012 | I/O nets HPWL: {:.2f} um. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1152) | 0013 | Internal error, placed more pins than exist ({} out of {}). |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L409) | 0015 | Macro [$inst getName] is not placed. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L165) | 0016 | Both -direction and -pin_names constraints not allowed. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L427) | 0017 | -hor_layers is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L433) | 0018 | -ver_layers is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L465) | 0019 | Design without pins. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1298) | 0020 | Manufacturing grid is not defined. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L473) | 0021 | Horizontal routing tracks not found for layer $hor_layer_name. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L491) | 0023 | Vertical routing tracks not found for layer $ver_layer_name. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1764) | 0024 | Number of IO pins ({}) exceeds maximum number of available positions ({}). |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L531) | 0025 | -exclude: $interval is an invalid region. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L534) | 0026 | -exclude: invalid syntax in $region. Use (top|bottom|left|right):interval. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L565) | 0027 | $cmd: $edge is an invalid edge. Use top, bottom, left or right. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L578) | 0028 | $cmd: Invalid pin direction. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L621) | 0029 | $cmd: Invalid edge |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L629) | 0030 | Invalid edge for command $cmd, should be one of top, bottom, left, right. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L394) | 0031 | No technology found. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L399) | 0032 | No block found. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/HungarianMatching.cpp#L124) | 0033 | I/O pin {} cannot be placed in the specified region. Not enough space. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1906) | 0034 | Pin {} has dimension {}u which is less than the min width {}u of layer {}. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1238) | 0036 | Number of sections is {} while the maximum recommended value is {} this may negatively affect performance. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1247) | 0037 | Number of slots per sections is {} while the maximum recommended value is {} this may negatively affect performance. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L2323) | 0038 | Pin {} without net. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1843) | 0039 | Assigned {} pins out of {} IO pins. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L815) | 0040 | Negative number of slots. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L542) | 0041 | Pin group $group_idx: \[$group\] |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1097) | 0042 | Unsuccessfully assigned I/O groups. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L549) | 0043 | Pin $pin_name not found in group $group_idx. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L224) | 0044 | Pin group: \[$final_group \] |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L477) | 0045 | Layer $hor_layer_name preferred direction is not horizontal. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L495) | 0046 | Layer $ver_layer_name preferred direction is not vertical. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L219) | 0047 | Group pin $pin_name not found in the design. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L657) | 0048 | Restrict pins \[$names\] to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u at the $edge_name edge. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L171) | 0049 | Restrict $direction pins to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u, in the $edge edge. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L646) | 0050 | No technology has been read. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L651) | 0051 | Layer $layer_name not found. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L55) | 0052 | Routing layer not found for name $layer_name. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L58) | 0053 | -layer is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L65) | 0054 | -x_step and -y_step are required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L87) | 0055 | -region is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L93) | 0056 | -size is not a list of 2 values. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L99) | 0057 | -size is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L208) | 0058 | The -pin_names argument is required when using -group flag. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L192) | 0059 | Box at top layer must have 4 values (llx lly urx ury). |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L671) | 0060 | Restrict pins \[$names\] to region ([ord::dbu_to_microns $llx]u, [ord::dbu_to_microns $lly]u)-([ord::dbu_to_microns $urx]u, [ord::dbu_to_microns $urx]u) at routing layer $top_layer_name. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L684) | 0061 | Pins for $cmd command were not found. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L83) | 0063 | -region is not a list of 4 values {llx lly urx ury}. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L326) | 0064 | -pin_name is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L332) | 0065 | -layer is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L338) | 0066 | -location is required. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L342) | 0068 | -location is not a list of 2 values. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L355) | 0069 | -pin_size is not a list of 2 values. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1987) | 0070 | Pin {} placed at ({}um, {}um). |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L363) | 0071 | Command place_pin should receive only one pin name. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L234) | 0072 | Number of pins ({}) exceed number of valid positions ({}). |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L200) | 0073 | Constraint with region $region has an invalid edge. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1557) | 0075 | Pin {} is assigned to more than one constraint, using last defined constraint. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1595) | 0076 | Constraint does not have available slots for its pins. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1322) | 0077 | Layer {} of Pin {} not found. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1082) | 0078 | Not enough available positions ({}) in section ({}, {})-({}, {}) at edge {} to place the pin group of size {}. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1393) | 0079 | Pin {} area {:2.4f}um^2 is lesser than the minimum required area {:2.4f}um^2. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L239) | 0080 | Mirroring pins $pin1 and $pin2. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L235) | 0081 | List of pins must have an even number of pins. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/HungarianMatching.cpp#L171) | 0082 | Mirrored position ({}, {}) at layer {} is not a valid position for pin {} placement. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L135) | 0083 | Both -region and -mirrored_pins constraints not allowed. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1541) | 0084 | Pin {} is mirrored with another pin. The constraint for this pin will be dropped. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L316) | 0085 | Mirrored position ({}, {}) at layer {} is not a valid position for pin placement. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/HungarianMatching.cpp#L323) | 0086 | Pin group of size {} was not assigned. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L139) | 0087 | Both -mirrored_pins and -group constraints not allowed. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1809) | 0088 | Cannot assign {} constrained pins to region {}u-{}u at edge {}. Not enough space in the defined region. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/HungarianMatching.cpp#L264) | 0089 | Could not create matrix for groups. Not available slots inside section. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L358) | 0090 | Group of size {} does not fit in constrained region. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L441) | 0091 | Mirrored position ({}, {}) at layer {} is not a valid position for pin {} placement. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1785) | 0092 | Pin group of size {} does not fit any section. Adding to fallback mode. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L384) | 0093 | Pin group of size {} does not fit in the constrained region {:.2f}-{:.2f} at {} edge. First pin of the group is {}. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L2378) | 0094 | Cannot create group of size {}. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L229) | 0095 | -order cannot be used without -group. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L227) | 0096 | Pin group of size {} does not fit constraint region. Adding to fallback mode. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L519) | 0097 | The max contiguous slots ({}) is smaller than the group size ({}). |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L1635) | 0098 | Pins {} are assigned to multiple constraints. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.tcl#L667) | 0099 | Constraint up:{$llx $lly $urx $ury} cannot be created. Pin placement grid on top layer not created. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L554) | 0100 | Group of size {} placed during fallback mode. |
| [PPL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/ppl/src/IOPlacer.cpp#L466) | 0101 | Slot for position ({}, {}) in layer {} not found |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L105) | 0001 | Reading voltage source file: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L111) | 0002 | Output voltage file is specified as: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L124) | 0003 | Output current file specified {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L130) | 0004 | EM calculation is enabled. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L137) | 0005 | Output spice file is specified as: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L162) | 0006 | SPICE file is written at: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L165) | 0007 | Failed to write out spice file: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L151) | 0008 | Powergrid is not connected to all instances, therefore the IR Solver may not be accurate. LVS may also fail. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L175) | 0010 | LU factorization of the G Matrix failed. SparseLU solver message: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L186) | 0012 | Solving V = inv(G)*J failed. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L307) | 0014 | Number of voltage sources cannot be 0. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L324) | 0015 | Reading location of VDD and VSS sources from {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L355) | 0016 | Voltage pad location (VSRC) file not specified, defaulting pad location to checkerboard pattern on core area. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L369) | 0017 | X direction bump pitch is not specified, defaulting to {}um. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L377) | 0018 | Y direction bump pitch is not specified, defaulting to {}um. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L386) | 0019 | Voltage on net {} is not explicitly set. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L391) | 0020 | Cannot find net {} in the design. Please provide a valid VDD/VSS net. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L400) | 0021 | Using voltage {:4.3f}V for ground network. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L406) | 0022 | Using voltage {:4.3f}V for VDD network. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L568) | 0024 | Instance {}, current node at ({}, {}) at layer {} have been moved from ({}, {}). |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1187) | 0027 | Cannot find net {} in the design. Please provide a valid VDD/VSS net. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1094) | 0030 | VSRC location at ({:4.3f}um, {:4.3f}um) and size {:4.3f}um, is not located on an existing power stripe node. Moving to closest node at ({:4.3f}um, {:4.3f}um). |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1219) | 0031 | Number of PDN nodes on net {} = {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L972) | 0032 | Node at ({}, {}) and layer {} moved from ({}, {}). |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L983) | 0033 | Node at ({}, {}) and layer {} moved from ({}, {}). |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L952) | 0035 | {} resistance not found in DB. Check the LEF or set it using the 'set_layer_rc' command. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1005) | 0036 | Layer {} per-unit resistance not found in DB. Check the LEF or set it using the command 'set_layer_rc -layer'. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1026) | 0037 | Layer {} per-unit resistance not found in DB. Check the LEF or set it using the command 'set_layer_rc -layer'. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1296) | 0038 | Unconnected PDN node on net {} at location ({:4.3f}um, {:4.3f}um), layer: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1317) | 0039 | Unconnected instance {} at location ({:4.3f}um, {:4.3f}um) layer: {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1330) | 0040 | All PDN stripes on net {} are connected. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1370) | 0041 | Could not open SPICE file {}. Please check if it is a valid path. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L549) | 0042 | Unable to connect macro/pad Instance {} to the power grid. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L191) | 0045 | Layer {} contains no grid nodes. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L201) | 0046 | Node location lookup error for y. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L203) | 0047 | Node location lookup error for x. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L285) | 0048 | Printing GMat obj, with {} nodes. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L337) | 0049 | No nodes in object, initialization stopped. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L391) | 0050 | Creating stripe condunctance with invalid inputs. Min and max values for X or Y are interchanged. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L566) | 0051 | Index out of bound for getting G matrix conductance. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L592) | 0052 | Index out of bound for getting G matrix conductance. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L57) | 0053 | Cannot read $vsrc_file. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L64) | 0054 | Argument -net not specified. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L102) | 0055 | EM outfile defined without EM enable flag. Add -enable_em. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L108) | 0056 | No rows defined in design. Floorplan not defined. Use initialize_floorplan to add rows. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L122) | 0057 | Argument -net not specified. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L131) | 0058 | No rows defined in design. Use initialize_floorplan to add rows. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L151) | 0059 | Cannot read $vsrc_file. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L162) | 0060 | Argument -net not specified. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L176) | 0061 | No rows defined in design. Use initialize_floorplan to add rows and construct PDN. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L192) | 0062 | Argument -net or -voltage not specified. Please specify both -net and -voltage arguments. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L416) | 0063 | Specified bump pitches of {:4.3f} and {:4.3f} are less than core width of {:4.3f} or core height of {:4.3f}. Changing bump location to the center of the die at ({:4.3f}, {:4.3f}). |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L309) | 0064 | Number of voltage sources = {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L435) | 0065 | VSRC location not specified, using default checkerboard pattern with one VDD every size bumps in x-direction and one in two bumps in the y-direction |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1049) | 0066 | Layer {} per-unit resistance not found in DB. Check the LEF or set it using the command 'set_layer_rc -layer'. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1112) | 0067 | Multiple voltage supply values mappedat the same node ({:4.3f}um, {:4.3f}um).If you provided a vsrc file. Check for duplicate entries.Choosing voltage value {:4.3f}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L282) | 0068 | Minimum resolution not set. Please run analyze_power_grid first. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L127) | 0069 | Check connectivity failed. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1209) | 0070 | Net {} has no nodes and will be skipped |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L462) | 0071 | Instance {} is not placed. Therefore, the power drawn by this instance is not considered for IR drop estimation. Please run analyze_power_grid after instances are placed. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L538) | 0072 | No nodes found in macro/pad bounding box for Instance {}.Using nearest node at ({}, {}) on the pin layer at routing level {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1154) | 0073 | Setting lower metal node density to {}um as specfied by user. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L519) | 0074 | No nodes found in macro or pad bounding box for Instance {} for the pin layer at routing level {}. Using layer {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L348) | 0075 | Expected four values on line: {} |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1175) | 0076 | Setting metal node density to be standard cell height times {}. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.tcl#L76) | 0077 | Cannot use both node_density and node_density_factor together. Use any one argument |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L191) | 0078 | IR drop setup failed. Analysis can't proceed. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/get_voltage.cpp#L58) | 0079 | Can't determine the supply voltage as no Liberty is loaded. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/gmat.cpp#L111) | 0080 | Creating stripe condunctance with invalid inputs. Min and max values for X or Y are interchanged. |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L715) | 0081 | Via connection failed at {}, {} |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/ir_solver.cpp#L1169) | 0082 | Unable to find a row |
| [PSM](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/psm/src/pdnsim.cpp#L118) | 0083 | Error file is specified as: {}. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L312) | 0001 | Reading SPEF file: {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L320) | 0002 | Filename is not defined! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2272) | 0003 | Read SPEF into extracted db! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2268) | 0004 | There is no extraction db! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2283) | 0005 | Can't open SPEF file {} to write. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extBench.cpp#L450) | 0007 | Finished {} measurements for pattern MET_UNDER_MET |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L233) | 0008 | extracting parasitics of {} ... |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L250) | 0015 | Finished extracting {}. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L283) | 0016 | Writing SPEF ... |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L306) | 0017 | Finished writing SPEF ... |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L369) | 0019 | diffing spef {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L422) | 0021 | calibrate on spef file {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L187) | 0029 | Defined extraction corner {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L198) | 0030 | The original process corner name is required |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L207) | 0031 | Defined Derived extraction corner {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1834) | 0040 | Final {} rc segments |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L1761) | 0042 | {} nets finished |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extFlow.cpp#L1179) | 0043 | {} wires to be extracted |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2624) | 0044 | {} spef insts not found in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1935) | 0045 | Extract {} nets, {} rsegs, {} caps, {} ccs |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L1801) | 0047 | {} nets finished |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2618) | 0048 | {} db nets not read from spef. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2620) | 0049 | {} db insts not read from spef. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2622) | 0050 | {} spef nets not found in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2627) | 0052 | Unmatched spef and db! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extBench.cpp#L282) | 0055 | Finished {} bench measurements for pattern MET_OVER_MET |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extBench.cpp#L335) | 0057 | Finished {} bench measurements for pattern MET_UNDER_MET |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extBench.cpp#L390) | 0058 | Finished {} bench measurements for pattern MET_DIAGUNDER_MET |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2570) | 0060 | merged {} coupling caps |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L746) | 0065 | layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3292) | 0069 | {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3407) | 0072 | Can't find <OVER> rules for {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmeasure.cpp#L66) | 0074 | Cannot find net {} from the {} table entry {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L549) | 0076 | Cap Node {} not extracted |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L764) | 0077 | Spef net {} not found in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2214) | 0078 | Break simple loop of {} nets |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmeasure.cpp#L1757) | 0079 | Have processed {} CC caps, and stored {} CC caps |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extFlow.cpp#L275) | 0081 | Die Area for the block has 0 size, or is undefined! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extFlow.cpp#L312) | 0082 | Layer {}, routing level {}, has pitch {}!! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1943) | 0107 | Nothing is extracted out of {} nets! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L459) | 0108 | Net {} multiple-ended at bterm {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L472) | 0109 | Net {} multiple-ended at iterm {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L617) | 0110 | Net {} has no wires. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L670) | 0111 | Net {} {} has a loop at x={} y={} {}. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L762) | 0112 | Can't locate bterm {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L766) | 0113 | Can't locate iterm {}/{} ( {} ) |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L773) | 0114 | Net {} {} does not start from an iterm or a bterm. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L779) | 0115 | Net {} {} already has rseg! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1193) | 0120 | No matching process corner for scaled corner {}, model {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1333) | 0121 | The corresponding process corner has to be defined using the command <define_process_corner> |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1348) | 0122 | A process corner for Extraction RC Model {} has already been defined, skipping definition |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1742) | 0127 | No RC model was read with command <load_model>, will not perform extraction! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1764) | 0128 | skipping Extraction ... |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1768) | 0129 | Wrong combination of corner related options! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2167) | 0134 | Can't execute write_spef command. There's no extraction data. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2093) | 0135 | Corner {} is out of range; There are {} corners in DB! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2116) | 0136 | Can't find corner name {} in the parasitics DB! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2186) | 0137 | Can't open file \"{}\" to write spef. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L440) | 0138 | {} layers are missing resistance value; Check LEF file. Extraction cannot proceed! Exiting |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L432) | 0139 | Missing Resistance value for layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L643) | 0140 | Have processed {} total segments, {} signal segments, {} CC caps, and stored {} CC caps |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L725) | 0141 | Context of layer {} xy={} len={} base={} width={} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L737) | 0142 | layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L748) | 0143 | {}: {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L172) | 0147 | bench_verilog: file is not defined! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L226) | 0148 | Extraction corner {} not found! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/OpenRCX.tcl#L329) | 0149 | -db is deprecated. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extprocess.cpp#L287) | 0152 | Can't determine Top Width for Conductor <{}> |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extprocess.cpp#L317) | 0153 | Can't determine thickness for Conductor <{}> |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extprocess.cpp#L518) | 0154 | Can't determine thickness for Diel <{}> |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extprocess.cpp#L264) | 0158 | Can't determine Bottom Width for Conductor <{}> |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extprocess.cpp#L534) | 0159 | Can't open file {} with permissions <{}> |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L319) | 0171 | Can't open log file diff_spef.log for writing! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L323) | 0172 | Can't open output file diff_spef.out for writing! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L1301) | 0175 | Non-symmetric case feature is not implemented! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L1595) | 0176 | Skip instance {} for cell {} is excluded |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2295) | 0178 | \" -N {} \" is unknown. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L147) | 0208 | {} {} {} {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3417) | 0216 | Can't find <UNDER> rules for {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3422) | 0217 | Can't find <OVERUNDER> rules for {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3468) | 0218 | Cannot write <OVER> rules for <DensityModel> {} and layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3481) | 0219 | Cannot write <UNDER> rules for <DensityModel> {} and layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3500) | 0220 | Cannot write <DIAGUNDER> rules for <DensityModel> {} and layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3513) | 0221 | Cannot write <OVERUNDER> rules for <DensityModel> {} and layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3746) | 0222 | There were {} extraction models defined but only {} exists in the extraction rules file {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3769) | 0223 | Cannot find model index {} in extRules file {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extFlow.cpp#L1323) | 0239 | Zero rseg wire property {} on main net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extFlow.cpp#L1334) | 0240 | GndCap: cannot find rseg for rsegId {} on net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L54) | 0252 | Ext object on dbBlock is NULL! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L137) | 0258 | Spef instance {} not found in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L170) | 0259 | Can't find bterm {} in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L433) | 0260 | {} and {} are connected to a coupling cap of net {} {} in spef, but connected to net {} {} and net {} {} respectively in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L599) | 0261 | Cap Node {} not extracted |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L523) | 0262 | Iterm {}/{} is connected to net {} {} in spef, but connected to net {} {} in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L575) | 0263 | Bterm {} is connected to net {} {} in spef, but connected to net {} {} in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L901) | 0264 | Spef net {} not found in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1090) | 0265 | There is cc cap between net {} and net {} in db, but not in reference spef file |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1147) | 0266 | There is cc cap between net {} and net {} in reference spef file, but not in db |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1339) | 0267 | {} has no shapes! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1492) | 0269 | Cannot find coords of driver capNode {} of net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1535) | 0270 | Driving node of net {} {} is not connected to a rseg. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1612) | 0271 | Cannot find node coords for targetCapNodeId {} of net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1629) | 0272 | RC of net {} {} is disconnected! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1669) | 0273 | Failed to identify loop in net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1675) | 0274 | {} capNodes loop in net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1683) | 0275 | id={} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1685) | 0276 | cap-{}={} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1727) | 0277 | Break one simple loop of {}-rsegs net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1753) | 0278 | {}-rsegs net {} {} has a {}-rsegs loop |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1779) | 0279 | {}-rsegs net {} {} has {} loops |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1832) | 0280 | Net {} {} has rseg before reading spef |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1881) | 0281 | \"-N s\" in read_spef command, but no coordinates in spef file. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1958) | 0282 | Source capnode {} is the same as target capnode {}. Add the cc capacitance to ground. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2206) | 0283 | Have read {} nets |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2212) | 0284 | There are {} nets with looped spef rc |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2527) | 0285 | Break simple loop of {} nets |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2336) | 0286 | Number of corners in SPEF file = 0. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2355) | 0287 | Cannot find corner name {} in DB |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2362) | 0288 | Ext corner {} out of range; There are only {} defined process corners. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2376) | 0289 | Mismatch on the numbers of corners: Spef file has {} corners vs. Process corner table has {} corners.(Use -spef_corner option). |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2386) | 0290 | Spef corner {} out of range; There are only {} corners in Spef file |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2421) | 0291 | Have to specify option _db_corner_name |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2525) | 0292 | {} nets with looped spef rc |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2640) | 0293 | *{}{}{} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2657) | 0294 | First {} cc that appear {} times |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2756) | 0295 | There is no *PORTS section |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2783) | 0296 | There is no *NAME_MAP section |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2788) | 0297 | There is no *NAME_MAP section |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2790) | 0298 | There is no *PORTS section |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/OpenRCX.tcl#L232) | 0357 | No LEF technology has been read. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3412) | 0358 | Can't find <RESOVER> Res rules for {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1596) | 0374 | Inconsistency in RC of net {} {}. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2330) | 0376 | DB created {} nets, {} rsegs, {} caps, {} ccs |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L176) | 0378 | Can't open file {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L366) | 0380 | Filename is not defined to run diff_spef command! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/ext.cpp#L417) | 0381 | Filename for calibration is not defined. Define the filename using -spef_file |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2346) | 0404 | Cannot find corner name {} in DB |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1771) | 0405 | {}-rsegs net {} {} has {} loops |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1720) | 0406 | Break one simple loop of {}-rsegs net {} {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L1761) | 0407 | {}-rsegs net {} {} has a {}-rsegs loop |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3455) | 0410 | Cannot write <OVER> Res rules for <DensityModel> {} and layer {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L2647) | 0411 | \ttotFrCap for netId {}({}) {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L2641) | 0414 | \tfrCap from CC for netId {}({}) {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L2623) | 0416 | \tccCap for netIds {}({}), {}({}) {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L2605) | 0417 | FrCap for netId {} (nodeId= {}) {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L2581) | 0418 | Reads only {} nodes from {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1292) | 0431 | Defined process_corner {} with ext_model_index {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1268) | 0433 | A process corner {} for Extraction RC Model {} has already been defined, skipping definition |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1298) | 0434 | Defined process_corner {} with ext_model_index {} (using extRulesFile defaults) |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1624) | 0435 | Reading extraction model file {} ... |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1807) | 0436 | RC segment generation {} (max_merge_res {:.1f}) ... |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L78) | 0437 | RECT {} ( {} {} ) ( {} {} ) jids= ( {} {} ) |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L67) | 0438 | VIA {} ( {} {} ) jids= ( {} {} ) |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1838) | 0439 | Coupling Cap extraction {} ... |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1849) | 0440 | Coupling threshhold is {:.4f} fF, coupling capacitance less than {:.4f} fF will be grounded. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extFlow.cpp#L1293) | 0442 | {:d}% completion -- {:d} wires have been extracted |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L1765) | 0443 | {} nets finished |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2223) | 0444 | Read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2497) | 0445 | Have read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2607) | 0447 | Db inst {} {} not read from spef file! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2592) | 0448 | Db net {} {} not read from spef file! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2647) | 0449 | {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2650) | 0451 | {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L69) | 0452 | Spef instance {} not found in db. |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmeasure.cpp#L913) | 0456 | pixelTable gave len {}, bigger than expected {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmeasure.cpp#L974) | 0458 | pixelTable gave len {}, bigger than expected {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmeasure.cpp#L603) | 0459 | getOverUnderIndex: out of range n= {} m={} u= {} o= {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmeasure.cpp#L76) | 0460 | Cannot find dbRseg for net {} from the {} table entry {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2560) | 0463 | Have read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpefIn.cpp#L2572) | 0464 | Broke {} coupling caps of {} fF or smaller |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extSpef.cpp#L1799) | 0465 | {} nets finished |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1629) | 0468 | Can't open extraction model file {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1317) | 0472 | The corresponding process corner has to be defined using the command <define_process_corner> |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2055) | 0474 | Can't execute write_spef command. There's no block in db! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2150) | 0475 | Can't execute write_spef command. There's no block in db |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L739) | 0476 | {}: {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L2347) | 0480 | cc appearance count -- 1:{} 2:{} 3:{} 4:{} 5:{} 6:{} 7:{} 8:{} 9:{} 10:{} 11:{} 12:{} 13:{} 14:{} 15:{} 16:{} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L2519) | 0485 | Cannot open file {} with permissions {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/netRC.cpp#L1650) | 0487 | No RC model read from the extraction model! Ensure the right extRules file is used! |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3005) | 0489 | mv failed: {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3294) | 0490 | system failed: {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extRCmodel.cpp#L3303) | 0491 | rm failed on {} |
| [RCX](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/src/extmain.cpp#L471) | 0497 | No design is loaded. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L105) | 0001 | Cannot open file {}. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L357) | 0002 | Blif writer successfully dumped file with {} instances. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L381) | 0003 | Cannot open file {}. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L404) | 0004 | Cannot open file {}. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L426) | 0005 | Blif parsed successfully, will destroy {} existing instances. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L430) | 0006 | Found {} inputs, {} outputs, {} clocks, {} combinational gates, {} registers after parsing the blif file. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L455) | 0007 | Inserting {} new instances. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L473) | 0008 | Const driver {} doesn't have any connected nets. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L516) | 0009 | Master ({}) not found while stitching back instances. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L569) | 0010 | Connection {} parsing failed for {} instance. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/rmp.tcl#L90) | 0012 | Missing argument -liberty_file |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/Restructure.cpp#L618) | 0016 | cannot open file {} |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/Restructure.cpp#L483) | 0020 | Cannot open file {} for writing. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/Restructure.cpp#L292) | 0021 | All re-synthesis runs discarded, keeping original netlist. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/Restructure.cpp#L640) | 0025 | ABC run failed, see log file {} for details. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/Restructure.cpp#L225) | 0026 | Error executing ABC command {}. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/rmp.tcl#L106) | 0032 | -tielo_port not specified |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/rmp.tcl#L122) | 0033 | -tiehi_port not specified |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L418) | 0034 | Blif parser failed. File doesn't follow blif spec. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/Restructure.cpp#L451) | 0035 | Could not create instance {}. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/Restructure.cpp#L592) | 0036 | Mode {} not recognized. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L536) | 0076 | Could not create new instance of type {} with name {}. |
| [RMP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rmp/src/blif.cpp#L590) | 0146 | Could not connect instance of cell type {} to {} net due to unknown mterm in blif. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L145) | 0001 | Use -layer or -resistance/-capacitance but not both. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L150) | 0002 | layer $layer_name not found. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L237) | 0003 | missing -placement or -global_routing flag. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L549) | 0004 | -max_utilization must be between 0 and 100%. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L234) | 0005 | Run global_route before estimating parasitics for global routing. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L198) | 0010 | $signal_clk wire resistance is 0. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L201) | 0011 | $signal_clk wire capacitance is 0. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L571) | 0014 | wire capacitance for corner [$corner name] is zero. Use the set_wire_rc command to set wire resistance and capacitance. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L480) | 0020 | found $floating_net_count floating nets. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L516) | 0021 | no estimated parasitics. Using wire load models. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L413) | 0022 | no buffers found. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L283) | 0025 | max utilization reached. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L311) | 0026 | Removed {} buffers. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L463) | 0027 | Inserted {} input buffers. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L591) | 0028 | Inserted {} output buffers. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L307) | 0030 | Inserted {} buffers. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L309) | 0031 | Resized {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairHold.cc#L300) | 0032 | Inserted {} hold buffers. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairHold.cc#L309) | 0033 | No hold violations found. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L118) | 0034 | Found {} slew violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L120) | 0035 | Found {} fanout violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L122) | 0036 | Found {} capacitance violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L124) | 0037 | Found {} long wires. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L126) | 0038 | Inserted {} buffers in {} nets. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L130) | 0039 | Resized {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L262) | 0040 | Inserted {} buffers. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L270) | 0041 | Resized {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L1628) | 0042 | Inserted {} tie {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L273) | 0043 | Swapped pins on {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L311) | 0044 | Swapped pins on {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L265) | 0045 | Inserted {} buffers, {} to split loads. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairHold.cc#L268) | 0046 | Found {} endpoints with hold violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L237) | 0047 | Found {} long wires. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L239) | 0048 | Inserted {} buffers in {} nets. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L276) | 0049 | Cloned {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairHold.cc#L306) | 0050 | Max utilization reached. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L286) | 0051 | Found {} slew violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L288) | 0052 | Found {} fanout violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L290) | 0053 | Found {} capacitance violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L292) | 0054 | Found {} long wires. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L294) | 0055 | Inserted {} buffers in {} nets. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L300) | 0056 | Resized {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L302) | 0057 | Resized {} instances. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L587) | 0058 | Using max wire length [format %.0f [sta::distance_sta_ui $max_wire_length]]um. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairHold.cc#L304) | 0060 | Max buffer count reached. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L163) | 0061 | $signal_clk wire resistance [sta::format_resistance [expr $wire_res * 1e-6] 6] [sta::unit_scale_abbreviation resistance][sta::unit_suffix resistance]/um capacitance [sta::format_capacitance [expr $wire_cap * 1e-6] 6] [sta::unit_scale_abbreviation capacitance][sta::unit_suffix capacitance]/um. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairSetup.cc#L280) | 0062 | Unable to repair all setup violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairHold.cc#L297) | 0064 | Unable to repair all hold checks within margin. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L583) | 0065 | max wire length less than [format %.0fu [sta::distance_sta_ui $min_delay_max_wire_length]] increases wire delays. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairHold.cc#L295) | 0066 | Unable to repair all hold violations. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L527) | 0067 | $key must be between 0 and 100 percent. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L1366) | 0068 | missing target load cap. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/SteinerTree.cc#L92) | 0069 | skipping net {} with {} pins. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L2090) | 0070 | no LEF cell for {}. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Rebuffer.cc#L288) | 0071 | unhandled BufferedNet type |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L613) | 0072 | unhandled BufferedNet type |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/BufferedNet.cc#L647) | 0073 | driver pin {} not found in global routes |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/BufferedNet.cc#L651) | 0074 | driver pin {} not found in global route grid points |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Rebuffer.cc#L138) | 0075 | makeBufferedNet failed for driver {} |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.tcl#L414) | 0076 | -slack_margin is deprecated. Use -setup_margin/-hold_margin |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L2754) | 0077 | some buffers were moved inside the core. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/BufferedNet.cc#L83) | 0078 | incorrect BufferedNet type {} |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/BufferedNet.cc#L117) | 0079 | incorrect BufferedNet type {} |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/BufferedNet.cc#L144) | 0080 | incorrect BufferedNet type {} |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/BufferedNet.cc#L173) | 0081 | incorrect BufferedNet type {} |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/BufferedNet.cc#L342) | 0082 | wireRC called for non-wire |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/RepairDesign.cc#L1000) | 0083 | pin outside regions |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L609) | 0084 | Output {} can't be buffered due to dont-touch driver {} |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L518) | 0085 | Input {} can't be buffered due to dont-touch fanout {} |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L1243) | 0086 | metersToDbu({}) cannot convert negative distances |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L2047) | 0088 | Corner: {} has no wire signal resistance value. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/Resizer.cc#L2062) | 0089 | Could not find a resistance value for any corner. Cannot evaluate max wire length for buffer. Check over your `set_wire_rc` configuration |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/SteinerTree.cc#L394) | 0092 | Steiner tree creation error. |
| [RSZ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rsz/src/SteinerTree.cc#L323) | 0093 | Invalid Steiner point {} requested. 0 <= Valid values < {}. |
| [STA](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/dbSta/src/dbSta.cc#L519) | 1000 | instance {} swap master {} is not equivalent |
| [STT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/stt/src/SteinerTreeBuilder.tcl#L49) | 0001 | The alpha value must be between 0.0 and 1.0. |
| [STT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/stt/src/SteinerTreeBuilder.tcl#L72) | 0002 | set_routing_alpha: Wrong number of arguments. |
| [STT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/stt/src/SteinerTreeBuilder.tcl#L86) | 0003 | Nets for $cmd command were not found |
| [STT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/stt/src/SteinerTreeBuilder.cpp#L248) | 0004 | Net {} is connected to unplaced instance {}. |
| [STT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/stt/src/SteinerTreeBuilder.cpp#L267) | 0005 | Net {} is connected to unplaced pin {}. |
| [STT](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/stt/src/SteinerTreeBuilder.tcl#L102) | 0006 | Clock nets for $cmd command were not found |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L281) | 0004 | Inserted {} endcaps. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L420) | 0005 | Inserted {} tapcells. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L552) | 0006 | Inserted {} top/bottom cells. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L887) | 0007 | Inserted {} cells near blockages. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L262) | 0009 | Row {} has enough space for only one endcap. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L169) | 0010 | Master $tapcell_master_name not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L179) | 0011 | Master $endcap_master_name not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L181) | 0012 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L187) | 0013 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L74) | 0014 | endcap_cpp option is deprecated. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L143) | 0015 | tbtie_cpp option is deprecated. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L147) | 0016 | no_cell_at_top_bottom option is deprecated. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L476) | 0018 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L482) | 0019 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L487) | 0020 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L492) | 0021 | Master $tap_nwouttie_master_name not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L497) | 0022 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L503) | 0023 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L678) | 0024 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L684) | 0025 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L689) | 0026 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L694) | 0027 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L700) | 0028 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L706) | 0029 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L712) | 0030 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L718) | 0031 | Master {} not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L1024) | 0032 | Macro {} is not placed. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.cpp#L1050) | 0033 | Not able to build instance {} with master {}. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L213) | 0034 | Master $endcap_master_name not found. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L246) | 0100 | Removed $taps_removed tapcells. |
| [TAP](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/tap/src/tapcell.tcl#L248) | 0101 | Removed $endcaps_removed endcaps. |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.tcl#L288) | 0001 | -area is a list of 4 coordinates |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.tcl#L296) | 0002 | please define area |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L50) | 10001 | Creation of '%s' power domain failed |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L66) | 10002 | Couldn't retrieve power domain '%s' while adding element '%s' |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L84) | 10003 | Creation of '%s' logic port failed |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L99) | 10004 | Couldn't retrieve power domain '%s' while creating power switch '%s' |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L110) | 10005 | Creation of '%s' power switch failed |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L128) | 10006 | Couldn't retrieve power switch '%s' while adding control port '%s' |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L148) | 10007 | Couldn't retrieve power switch '%s' while adding on state '%s' |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L173) | 10008 | Couldn't retrieve power domain '%s' while creating/updating isolation '%s' |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L184) | 10009 | Couldn't update a non existing isolation %s |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L229) | 10010 | Couldn't retrieve power domain '%s' while updating isolation '%s' |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L240) | 10011 | Couldn't find isolation %s |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L260) | 10012 | Couldn't retrieve power domain '%s' while updating its area |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L330) | 10013 | isolation cell has no enable port |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L349) | 10014 | unknown isolation cell function |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L374) | 10015 | multiple power domain definitions for the same path %s |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L393) | 10016 | Creation of '%s' region failed |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L403) | 10017 | No area specified for '%s' power domain |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L411) | 10018 | Creation of '%s' group failed, duplicate group exists. |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L430) | 10019 | Creation of '{}' dbNet from UPF Logic Port failed |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L534) | 10020 | Isolation %s defined, but no cells defined. |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L558) | 10021 | Isolation %s cells defined, but can't find any in the lib. |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L594) | 10022 | Isolation %s cells defined, but can't find one of output, data or enable terms. |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L730) | 10023 | Isolation {} has nonexisting control net {} |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L778) | 10024 | Isolation %s has location %s, but only self|parent|fanoutsupported, defaulting to self. |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L842) | 10025 | No TOP DOMAIN found, aborting |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L879) | 10026 | Multiple isolation strategies defined for the same power domain %s. |
| [UPF](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/upf/src/upf.cpp#L907) | 10027 | can't find any inverters |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/CFileUtils.cpp#L8) | 0001 | seeking file to start {} |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/CFileUtils.cpp#L25) | 0002 | error reading {} bytes from file at offset {}: {} |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/CFileUtils.cpp#L35) | 0003 | read no bytes from file at offset {}, but neither error nor EOF |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/CFileUtils.cpp#L56) | 0004 | error writing {} bytes from file at offset {}: {} |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/ScopedTemporaryFile.cpp#L12) | 0005 | could not create temp file |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/ScopedTemporaryFile.cpp#L15) | 0006 | ScopedTemporaryFile; fd: {} path: {} |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/ScopedTemporaryFile.cpp#L19) | 0007 | could not open temp descriptor as FILE |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/ScopedTemporaryFile.cpp#L27) | 0008 | could not unlink temp file at {} |
| [UTL](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/utl/src/ScopedTemporaryFile.cpp#L31) | 0009 | could not close temp file: {} |
