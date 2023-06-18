# OpenROAD Messages Glossary
Listed below are the OpenROAD warning/error codes you may encounter during running.

| Tool | Code | Message                                             |
| ---- | ---- | --------------------------------------------------- |
| ANT | 0001 | Found {} pin violations. |
| ANT | 0002 | Found {} net violations. |
| ANT | 0008 | No detailed or global routing found. Run global_route or detailed_route first. |
| ANT | 0009 | Net {} requires more than {} diodes per gate to repair violations. |
| ANT | 0010 | -report_filename is deprecated. |
| ANT | 0011 | -report_violating_nets is deprecated. |
| ANT | 0012 | Net {} not found. |
| ANT | 0013 | No THICKNESS is provided for layer {}.  Checks on this layer will not be correct. |
| ANT | 0014 | Skipped net {} because it is special. |
| CTS | 0001 | Running TritonCTS with user-specified clock roots: {}. |
| CTS | 0003 | Total number of Clock Roots: {}. |
| CTS | 0004 | Total number of Buffers Inserted: {}. |
| CTS | 0005 | Total number of Clock Subnets: {}. |
| CTS | 0006 | Total number of Sinks: {}. |
| CTS | 0007 | Net \"{}\" found for clock \"{}\". |
| CTS | 0008 | TritonCTS.cpp:488         TritonCTS found {} clock nets. |
| CTS | 0010 | Clock net \"{}\" has {} sinks. |
| CTS | 0012 | Minimum number of buffers in the clock path: {}. |
| CTS | 0013 | Maximum number of buffers in the clock path: {}. |
| CTS | 0014 | {} clock nets were removed/fixed. |
| CTS | 0015 | Created {} clock nets. |
| CTS | 0016 | Fanout distribution for the current clock = {}. |
| CTS | 0017 | Max level of the clock tree: {}. |
| CTS | 0018 | Created {} clock buffers. |
| CTS | 0019 | Total number of sinks after clustering: {}. |
| CTS | 0020 | Wire segment unit: {}  dbu ({} um). |
| CTS | 0021 | Distance between buffers: {} units ({} um). |
| CTS | 0022 | Branch length for Vertex Buffer: {} units ({} um). |
| CTS | 0023 | Original sink region: {}. |
| CTS | 0024 | Normalized sink region: {}. |
| CTS | 0025 | Width:  {:.4f}. |
| CTS | 0026 | Height: {:.4f}. |
| CTS | 0027 | Generating H-Tree topology for net {}. |
| CTS | 0028 | Total number of sinks: {}. |
| CTS | 0029 | Sinks will be clustered in groups of up to {} and with maximum cluster diameter of {:.1f} um. |
| CTS | 0030 | Number of static layers: {}. |
| CTS | 0031 | Stop criterion found. Min length of sink region is ({}). |
| CTS | 0032 | Stop criterion found. Max number of sinks is {}. |
| CTS | 0034 | Segment length (rounded): {}. |
| CTS | 0035 | Number of sinks covered: {}. |
| CTS | 0038 | Number of created patterns = {}. |
| CTS | 0039 | Number of created patterns = {}. |
| CTS | 0040 | Net was not found in the design for {}, please check. Skipping... |
| CTS | 0041 | Net \"{}\" has {} sinks. Skipping... |
| CTS | 0042 | Net \"{}\" has no sinks. Skipping... |
| CTS | 0043 | {} wires are pure wire and no slew degradation.\nTritonCTS forced slew degradation on these wires. |
| CTS | 0045 | Creating fake entries in the LUT. |
| CTS | 0046 | Number of wire segments: {}. |
| CTS | 0047 | Number of keys in characterization LUT: {}. |
| CTS | 0048 | Actual min input cap: {}. |
| CTS | 0049 | Characterization buffer is: {}. |
| CTS | 0055 | Missing argument -buf_list |
| CTS | 0056 | Error when finding -clk_nets in DB. |
| CTS | 0057 | Missing argument, user must enter at least one of -root_buf or -buf_list. |
| CTS | 0058 | Invalid parameters in {}. |
| CTS | 0065 | Normalized values in the LUT should be in the range [1, {}\n    Check the table above to see the normalization ranges and your     characterization configuration. |
| CTS | 0073 | Buffer not found. Check your -buf_list input. |
| CTS | 0074 | Buffer {} not found. Check your -buf_list input. |
| CTS | 0075 | Error generating the wirelengths to test.\n    Check the -wire_unit parameter or the technology files. |
| CTS | 0076 | No Liberty cell found for {}. |
| CTS | 0078 | Error generating the wirelengths to test.\n    Check the parameters -max_cap/-max_slew/-cap_inter/-slew_inter\n          or the technology files. |
| CTS | 0079 | Sink not found. |
| CTS | 0080 | Sink not found. |
| CTS | 0081 | Buffer {} is not in the loaded DB. |
| CTS | 0082 | No valid clock nets in the design. |
| CTS | 0083 | No clock nets have been found. |
| CTS | 0084 | Compiling LUT. |
| CTS | 0085 | Could not find the root of {} |
| CTS | 0087 | Could not open output metric file {}. |
| CTS | 0090 | Sinks will be clustered based on buffer max cap. |
| CTS | 0093 | Fixing tree levels for max depth {} |
| CTS | 0095 | Net \"{}\" found. |
| CTS | 0096 | No Liberty cell found for {}. |
| CTS | 0097 | Characterization used {} buffer(s) types. |
| CTS | 0098 | Clock net \"{}\" |
| CTS | 0099 | Sinks {} |
| CTS | 0100 | Leaf buffers {} |
| CTS | 0101 | Average sink wire length {:.2f} um |
| CTS | 0102 | Path depth {} - {} |
| CTS | 0103 | No design block found. |
| CTS | 0104 | Clock wire resistance/capacitance values are zero.\nUse set_wire_rc to set them. |
| CTS | 0105 | Net \"{}\" already has clock buffer {}. Skipping... |
| CTS | 0106 | No Liberty found for buffer {}. |
| CTS | 0107 | No max slew found for cell {}. |
| CTS | 0108 | No max capacitance found for cell {}. |
| CTS | 0111 | No max capacitance found for cell {}. |
| CTS | 0113 | Characterization buffer is not defined.\n    Check that -buf_list has supported buffers from platform. |
| CTS | 0114 | Clock {} overlaps a previous clock. |
| CTS | 0115 | -post_cts_disable is obsolete. |
| CTS | 0534 | Could not find buffer input port for {}. |
| CTS | 0541 | Could not find buffer output port for {}. |
| DFT | 0002 | Can't scan replace cell '{:s}', that has lib cell '{:s}'. No scan equivalent lib cell found |
| DFT | 0003 | Cell '{:s}' is already an scan cell, we will not replace it |
| DFT | 0004 | ClockDomain.cpp:52        Clock mix config requested is not supported |
| DFT | 0004 | CellFactory.cpp:137   Cell '{:s}' doesn't have a valid clock connected. Can't create a scan cell |
| DFT | 0005 | CellFactory.cpp:146   Cell '{:s}' is not a scan cell. Can't use it for scan architect |
| DFT | 0005 | Requested clock mixing config not valid |
| DPL | 0001 | Placed {} filler instances. |
| DPL | 0002 | could not fill gap of size {} at {},{} dbu between {} and {} |
| DPL | 0012 | no rows found. |
| DPL | 0013 | Cannot paint grid because it is already occupied. |
| DPL | 0015 | instance {} does not fit inside the ROW core area. |
| DPL | 0016 | cannot place instance {}. |
| DPL | 0017 | cannot place instance {}. |
| DPL | 0020 | Mirrored {} instances |
| DPL | 0021 | HPWL before          {:8.1f} u |
| DPL | 0022 | HPWL after           {:8.1f} u |
| DPL | 0023 | HPWL delta           {:8.1f} % |
| DPL | 0026 | legalPt called on fixed cell. |
| DPL | 0027 | no rows defined in design. Use initialize_floorplan to add rows. |
| DPL | 0028 | $name did not match any masters. |
| DPL | 0029 | cannot find instance $inst_name |
| DPL | 0030 | cannot find instance $inst_name |
| DPL | 0031 | -max_displacement disp|{disp_x disp_y} |
| DPL | 0032 | Debug instance $instance_name not found. |
| DPL | 0033 | detailed placement checks failed. |
| DPL | 0034 | Detailed placement failed on the following {} instances: |
| DPL | 0035 | {} |
| DPL | 0036 | Detailed placement failed. |
| DPL | 0037 | Use remove_fillers before detailed placement. |
| DPL | 0038 | No 1-site fill cells detected.  To remove 1-site gaps use the -disallow_one_site_gaps flag. |
| DPL | 0039 | \"$arg\" did not match any masters. |
| This | could | due to a change from using regex to glob to search for cell masters. https://github.com/The-OpenROAD-Project/OpenROAD/pull/3210 |
| DPL | 0041 | Cannot paint grid because another layer is already occupied. |
| DPL | 0042 | No cells found in group {}.  |
| DPL | 0043 | No grid layers mapped. |
| DPL | 0044 | Cell {} with height {} is taller than any row. |
| DPO | 0001 | Unknown algorithm {:s}. |
| DPO | 0031 | -max_displacement disp|{disp_x disp_y} |
| DPO | 0100 | Creating network with {:d} cells, {:d} terminals, {:d} edges and {:d} pins. |
| DPO | 0101 | Unexpected total node count.  Expected {:d}, but got {:d} |
| DPO | 0102 | Improper node indexing while connecting pins. |
| DPO | 0103 | Could not find node for instance while connecting pins. |
| DPO | 0104 | Improper terminal indexing while connecting pins. |
| DPO | 0105 | Could not find node for terminal while connecting pins. |
| DPO | 0106 | Unexpected total edge count.  Expected {:d}, but got {:d} |
| DPO | 0107 | Unexpected total pin count.  Expected {:d}, but got {:d} |
| DPO | 0109 | Network stats: inst {}, edges {}, pins {} |
| DPO | 0110 | Number of regions is {:d} |
| DPO | 0200 | Unexpected displacement during legalization. |
| DPO | 0201 | Placement check failure during legalization. |
| DPO | 0202 | No movable cells found |
| DPO | 0203 | No movable cells found |
| DPO | 0300 | Set matching objective is {:s}. |
| DPO | 0301 | Pass {:3d} of matching; objective is {:.6e}. |
| DPO | 0302 | End of matching; objective is {:.6e}, improvement is {:.2f} percent. |
| DPO | 0303 | Running algorithm for {:s}. |
| DPO | 0304 | Pass {:3d} of reordering; objective is {:.6e}. |
| DPO | 0305 | End of reordering; objective is {:.6e}, improvement is {:.2f} percent. |
| DPO | 0306 | Pass {:3d} of global swaps; hpwl is {:.6e}. |
| DPO | 0307 | End of global swaps; objective is {:.6e}, improvement is {:.2f} percent. |
| DPO | 0308 | Pass {:3d} of vertical swaps; hpwl is {:.6e}. |
| DPO | 0309 | End of vertical swaps; objective is {:.6e}, improvement is {:.2f} percent. |
| DPO | 0310 | Assigned {:d} cells into segments.  Movement in X-direction is {:f}, movement in Y-direction is {:f}. |
| DPO | 0311 | Found {:d} overlaps between adjacent cells. |
| DPO | 0312 | Found {:d} edge spacing violations and {:d} padding violations. |
| DPO | 0313 | Found {:d} cells in wrong regions. |
| DPO | 0314 | Found {:d} site alignment problems. |
| DPO | 0315 | Found {:d} row alignment problems. |
| DPO | 0317 | ABU: Target {:.2f}, ABU_2,5,10,20: {:.2f}, {:.2f}, {:.2f}, {:.2f}, Penalty {:.2f} |
| DPO | 0318 | Collected {:d} single height cells. |
| DPO | 0319 | Collected {:d} multi-height cells spanning {:d} rows. |
| DPO | 0320 | Collected {:d} fixed cells (excluded terminal_NI). |
| DPO | 0321 | Collected {:d} wide cells. |
| DPO | 0322 | Image ({:d}, {:d}) - ({:d}, {:d}) |
| DPO | 0324 | Random improver is using {:s} generator. |
| DPO | 0325 | Random improver is using {:s} objective. |
| DPO | 0326 | Random improver cost string is {:s}. |
| DPO | 0327 | Pass {:3d} of random improver; improvement in cost is {:.2f} percent. |
| DPO | 0328 | End of random improver; improvement is {:.6f} percent. |
| DPO | 0329 | Random improver requires at least one generator. |
| DPO | 0330 | Test objective function failed, possibly due to a badly formed cost function. |
| DPO | 0332 | End of pass, Generator {:s} called {:d} times. |
| DPO | 0333 | End of pass, Objective {:s}, Initial cost {:.6e}, Scratch cost {:.6e}, Incremental cost {:.6e}, Mismatch? {:c} |
| DPO | 0334 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| DPO | 0335 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| DPO | 0336 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| DPO | 0337 | Generator {:s}, Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset. |
| DPO | 0338 | End of pass, Total cost is {:.6e}. |
| DPO | 0380 | Cell flipping. |
| DPO | 0381 | Encountered {:d} issues when orienting cells for rows. |
| DPO | 0382 | Changed {:d} cell orientations for row compatibility. |
| DPO | 0383 | Performed {:d} cell flips. |
| DPO | 0384 | End of flipping; objective is {:.6e}, improvement is {:.2f} percent. |
| DPO | 0385 | Only working with single height cells currently. |
| DPO | 0400 | Detailed improvement internal error: {:s}. |
| DPO | 0401 | Setting random seed to {:d}. |
| DPO | 0402 | Setting maximum displacement {:d} {:d} to {:d} {:d} units. |
| DRT | 0000 | initNetTerms unsupported obj. |
| DRT | 0002 | Detailed routing has not been run yet. |
| DRT | 0003 | Load design first. |
| DRT | 0004 | Load design first. |
| DRT | 0005 | Unsupported region query add. |
| DRT | 0006 | Unsupported region query add. |
| DRT | 0007 | Unsupported region query add. |
| DRT | 0008 | Unsupported region query add. |
| DRT | 0009 | Unsupported region query add. |
| DRT | 0010 | Unsupported region query add. |
| DRT | 0011 | Unsupported region query add. |
| DRT | 0012 | Unsupported region query add. |
| DRT | 0013 | Unsupported region query add. |
| DRT | 0014 | Unsupported region query add. |
| DRT | 0015 | Unsupported region query add. |
| DRT | 0016 | Unsupported region query add of blockage in instance {}. |
| DRT | 0017 | Unsupported region query add. |
| DRT | 0018 | Complete {} insts. |
| DRT | 0019 | Complete {} insts. |
| DRT | 0020 | Complete {} terms. |
| DRT | 0021 | Complete {} terms. |
| DRT | 0022 | Complete {} snets. |
| DRT | 0023 | Complete {} blockages. |
| DRT | 0024 | Complete {}. |
| DRT | 0026 | Complete {} origin guides. |
| DRT | 0027 | Complete {} origin guides. |
| DRT | 0028 | Complete {}. |
| DRT | 0029 | Complete {} nets (guide). |
| DRT | 0030 | Complete {} nets (guide). |
| DRT | 0031 | Unsupported region query add. |
| DRT | 0032 | {} grObj region query size = {}. |
| DRT | 0033 | {} shape region query size = {}. |
| DRT | 0034 | {} drObj region query size = {}. |
| DRT | 0035 | Complete {} (guide). |
| DRT | 0036 | {} guide region query size = {}. |
| DRT | 0037 | init_design_helper shape does not have net. |
| DRT | 0038 | init_design_helper shape does not have dr net. |
| DRT | 0039 | init_design_helper unsupported type. |
| DRT | 0041 | Unsupported metSpc rule. |
| DRT | 0042 | Unknown corner direction. |
| DRT | 0043 | Unsupported metSpc rule. |
| DRT | 0044 | Unsupported LEF58_SPACING rule for cut layer, skipped. |
| DRT | 0045 | Unsupported branch EXACTALIGNED in checkLef58CutSpacing_spc_adjCut. |
| DRT | 0046 | Unsupported branch EXCEPTSAMEPGNET in checkLef58CutSpacing_spc_adjCut. |
| DRT | 0047 | Unsupported branch EXCEPTALLWITHIN in checkLef58CutSpacing_spc_adjCut. |
| DRT | 0048 | Unsupported branch TO ALL in checkLef58CutSpacing_spc_adjCut. |
| DRT | 0050 | Unsupported branch ENCLOSURE in checkLef58CutSpacing_spc_adjCut. |
| DRT | 0051 | Unsupported branch SIDEPARALLELOVERLAP in checkLef58CutSpacing_spc_adjCut. |
| DRT | 0052 | Unsupported branch SAMEMASK in checkLef58CutSpacing_spc_adjCut. |
| DRT | 0053 | updateGCWorker cannot find frNet in DRWorker. |
| DRT | 0054 | Unsupported branch STACK in checkLef58CutSpacing_spc_layer. |
| DRT | 0055 | Unsupported branch ORTHOGONALSPACING in checkLef58CutSpacing_spc_layer. |
| DRT | 0056 | Unsupported branch SHORTEDGEONLY in checkLef58CutSpacing_spc_layer. |
| DRT | 0057 | Unsupported branch WIDTH in checkLef58CutSpacing_spc_layer. |
| DRT | 0058 | Unsupported branch PARALLEL in checkLef58CutSpacing_spc_layer. |
| DRT | 0059 | Unsupported branch EDGELENGTH in checkLef58CutSpacing_spc_layer. |
| DRT | 0060 | Unsupported branch EXTENSION in checkLef58CutSpacing_spc_layer. |
| DRT | 0061 | Unsupported branch ABOVEWIDTH in checkLef58CutSpacing_spc_layer. |
| DRT | 0062 | Unsupported branch MASKOVERLAP in checkLef58CutSpacing_spc_layer. |
| DRT | 0063 | Unsupported branch WRONGDIRECTION in checkLef58CutSpacing_spc_layer. |
| DRT | 0065 | instAnalysis unsupported pinFig. |
| DRT | 0066 | instAnalysis skips {} due to no pin shapes. |
| DRT | 0067 | FlexPA_prep.cpp:118       FlexPA mergePinShapes unsupported shape. |
| DRT | 0068 | prepPoint_pin_genPoints_rect cannot find secondLayerNum. |
| DRT | 0069 | initPinAccess error. |
| DRT | 0070 | Unexpected direction in getPlanarEP. |
| DRT | 0071 | prepPoint_pin_helper unique2paidx not found. |
| DRT | 0072 | prepPoint_pin_helper unique2paidx not found. |
| DRT | 0073 | No access point for {}/{}. |
| DRT | 0074 | No access point for PIN/{}. |
| DRT | 0075 | prepPoint_pin unique2paidx not found. |
| DRT | 0076 | Complete {} pins. |
| DRT | 0077 | Complete {} pins. |
| DRT | 0078 | Complete {} pins. |
| DRT | 0079 | Complete {} unique inst patterns. |
| DRT | 0080 | Complete {} unique inst patterns. |
| DRT | 0081 | Complete {} unique inst patterns. |
| DRT | 0082 | Complete {} groups. |
| DRT | 0083 | Complete {} groups. |
| DRT | 0084 | Complete {} groups. |
| DRT | 0085 | Valid access pattern combination not found for {} |
| DRT | 0086 | Pin does not have an access point. |
| DRT | 0087 | No valid pattern for unique instance {}, master is {}. |
| DRT | 0089 | genPattern_gc objs empty. |
| DRT | 0090 | Valid access pattern not found. |
| DRT | 0091 | Pin does not have valid ap. |
| DRT | 0092 | Duplicate diff layer samenet cut spacing, skipping cut spacing from {} to {}. |
| DRT | 0093 | Duplicate diff layer diffnet cut spacing, skipping cut spacing from {} to {}. |
| DRT | 0094 | Cannot find layer: {}. |
| DRT | 0095 | Library cell {} not found. |
| DRT | 0096 | Same cell name: {}. |
| DRT | 0097 | Cannot find cut layer {}. |
| DRT | 0098 | Cannot find bottom layer {}. |
| DRT | 0099 | Cannot find top layer {}. |
| DRT | 0100 | Unsupported via: {}. |
| DRT | 0101 | Non-consecutive layers for via: {}. |
| DRT | 0102 | Odd dimension in both directions. |
| DRT | 0103 | Unknown direction. |
| DRT | 0104 | Terminal {} not found. |
| DRT | 0105 | Component {} not found. |
| DRT | 0106 | Component pin {}/{} not found. |
| DRT | 0107 | Unsupported layer {}. |
| DRT | 0108 | Unsupported via in db. |
| DRT | 0109 | Unsupported via in db. |
| DRT | 0110 | Complete {} groups. |
| DRT | 0111 | Complete {} groups. |
| DRT | 0112 | Unsupported layer {}. |
| DRT | 0113 | Tech layers for via {} not found in db tech. |
| DRT | 0114 | Unknown connFig type while writing net {}. |
| DRT | 0115 | Setting MAX_THREADS=1 for use with the PA GUI. |
| DRT | 0116 | Load design first. |
| DRT | 0117 | Load design first. |
| DRT | 0118 | -worker is a list of 2 coordinates. |
| DRT | 0119 | Marker ({}, {}) ({}, {}) on {}: |
| DRT | 0122 | Layer {} is skipped for {}/{}. |
| DRT | 0123 | Layer {} is skipped for {}/OBS. |
| DRT | 0124 | Via {} with unused layer {} will be ignored. |
| DRT | 0125 | Unsupported via {}. |
| DRT | 0126 | Non-consecutive layers for via {}. |
| DRT | 0127 | Unknown layer {} for via {}. |
| DRT | 0128 | Unsupported viarule {}. |
| DRT | 0129 | Unknown layer {} for viarule {}. |
| DRT | 0130 | Non-consecutive layers for viarule {}. |
| DRT | 0131 | cutLayer cannot have overhangs in viarule {}, skipping enclosure. |
| DRT | 0132 | botLayer cannot have rect in viarule {}, skipping rect. |
| DRT | 0133 | topLayer cannot have rect in viarule {}, skipping rect. |
| DRT | 0134 | botLayer cannot have spacing in viarule {}, skipping spacing. |
| DRT | 0135 | botLayer cannot have spacing in viarule {}, skipping spacing. |
| DRT | 0136 | Load design first. |
| DRT | 0138 | New SPACING SAMENET overrides oldSPACING SAMENET rule. |
| DRT | 0139 | minEnclosedArea constraint with width is not supported, skipped. |
| DRT | 0140 | SpacingRange unsupported. |
| DRT | 0141 | SpacingLengthThreshold unsupported. |
| DRT | 0142 | SpacingNotchLength unsupported. |
| DRT | 0143 | SpacingEndOfNotchWidth unsupported. |
| DRT | 0144 | New SPACING SAMENET overrides oldSPACING SAMENET rule. |
| DRT | 0145 | New SPACINGTABLE PARALLELRUNLENGTH overrides old SPACING rule. |
| DRT | 0146 | New SPACINGTABLE TWOWIDTHS overrides old SPACING rule. |
| DRT | 0147 | cutWithin is smaller than cutSpacing for ADJACENTCUTS on layer {}, please check your rule definition. |
| DRT | 0148 | Deprecated lef param in params file. |
| DRT | 0149 | Reading tech and libs. |
| DRT | 0150 | Reading design. |
| DRT | 0153 | Cannot find net {}. |
| DRT | 0154 | Cannot find layer {}. |
| DRT | 0155 | Guide in net {} uses layer {} ({}) that is outside the allowed routing range [{} ({}), ({})]. |
| DRT | 0156 | guideIn read {} guides. |
| DRT | 0157 | guideIn read {} guides. |
| DRT | 0160 | Warning: {} does not have viaDef aligned with layer direction, generating new viaDef {}. |
| DRT | 0161 | Unsupported LEF58_SPACING rule for layer {} of type MAXXY. |
| DRT | 0162 | Library cell analysis. |
| DRT | 0163 | Instance analysis. |
| DRT | 0164 | Number of unique instances = {}. |
| DRT | 0165 | Start pin access. |
| DRT | 0166 | Complete pin access. |
| DRT | 0167 | List of default vias: |
| DRT | 0168 | Init region query. |
| DRT | 0169 | Post process guides. |
| DRT | 0170 | No GCELLGRIDX. |
| DRT | 0171 | No GCELLGRIDY. |
| DRT | 0172 | No GCELLGRIDX. |
| DRT | 0173 | No GCELLGRIDY. |
| DRT | 0174 | GCell cnt x < 1. |
| DRT | 0175 | GCell cnt y < 1. |
| DRT | 0176 | GCELLGRID X {} DO {} STEP {} ; |
| DRT | 0177 | GCELLGRID Y {} DO {} STEP {} ; |
| DRT | 0178 | Init guide query. |
| DRT | 0179 | Init gr pin query. |
| DRT | 0180 | Post processing. |
| DRT | 0181 | Start track assignment. |
| DRT | 0182 | Complete track assignment. |
| DRT | 0183 | Done with {} horizontal wires in {} frboxes and {} vertical wires in {} frboxes. |
| DRT | 0184 | Done with {} vertical wires in {} frboxes and {} horizontal wires in {} frboxes. |
| DRT | 0185 | Post process initialize RPin region query. |
| DRT | 0186 | Done with {} vertical wires in {} frboxes and {} horizontal wires in {} frboxes. |
| DRT | 0187 | Start routing data preparation. |
| DRT | 0194 | Start detail routing. |
| DRT | 0195 | Start {}{} optimization iteration. |
| DRT | 0198 | Complete detail routing. |
| DRT | 0199 | Number of violations = {}. |
| DRT | 0201 | Must load design before global routing. |
| DRT | 0202 | Skipping layer {} not found in db for congestion map. |
| DRT | 0203 | dbGcellGrid already exists in db. Clearing existing dbGCellGrid. |
| DRT | 0205 | Deprecated output param in params file. |
| DRT | 0206 | checkConnectivity error. |
| DRT | 0207 | Setting MAX_THREADS=1 for use with the DR GUI. |
| DRT | 0210 | Layer {} minWidth is larger than width. Using width as minWidth. |
| DRT | 0214 | genGuides empty pin2GCellMap. |
| DRT | 0215 | Pin {}/{} not covered by guide. |
| DRT | 0216 | Pin PIN/{} not covered by guide. |
| DRT | 0217 | genGuides unknown type. |
| DRT | 0218 | Guide is not connected to design. |
| DRT | 0219 | Guide is not connected to design. |
| DRT | 0220 | genGuides_final net {} error 1. |
| DRT | 0221 | genGuides_final net {} error 2. |
| DRT | 0222 | genGuides_final net {} pin not in any guide. |
| DRT | 0223 | Pin dangling id {} ({},{}) {}. |
| DRT | 0224 | {} {} pin not visited, number of guides = {}. |
| DRT | 0225 | {} {} pin not visited, fall back to feedthrough mode. |
| DRT | 0226 | Unsupported endofline spacing rule. |
| DRT | 0227 | Deprecated def param in params file. |
| DRT | 0228 | genGuides_merge cannot find touching layer. |
| DRT | 0229 | genGuides_split lineIdx is empty on {}. |
| DRT | 0230 | genGuides_gCell2TermMap avoid condition2, may result in guide open: {}. |
| DRT | 0231 | genGuides_gCell2TermMap avoid condition3, may result in guide open: {}. |
| DRT | 0232 | genGuides_gCell2TermMap unsupported pinfig. |
| DRT | 0234 | {} does not have single-cut via. |
| DRT | 0235 | Second layer {} does not exist. |
| DRT | 0236 | Updating diff-net cut spacing rule between {} and {}. |
| DRT | 0237 | Second layer {} does not exist. |
| DRT | 0238 | Updating same-net cut spacing rule between {} and {}. |
| DRT | 0239 | Non-rectangular shape in via definition. |
| DRT | 0240 | CUT layer {} does not have square single-cut via, cut layer width may be set incorrectly. |
| DRT | 0241 | CUT layer {} does not have single-cut via, cut layer width may be set incorrectly. |
| DRT | 0242 | CUT layer {} does not have default via. |
| DRT | 0243 | Non-rectangular shape in via definition. |
| DRT | 0244 | CUT layer {} has smaller width defined in LEF compared to default via. |
| DRT | 0245 | skipped writing guide updates to database. |
| DRT | 0246 | {}/{} from {} has nullptr as prefAP. |
| DRT | 0247 | io::Writer::fillConnFigs_net does not support this type. |
| DRT | 0248 | instAnalysis unsupported pinFig. |
| DRT | 0249 | Net {} (id = {}). |
| DRT | 0250 | Pin {}. |
| DRT | 0251 | -param cannot be used with other arguments |
| DRT | 0252 | params file is deprecated. Use tcl arguments. |
| DRT | 0253 | Design and tech mismatch. |
| DRT | 0255 | Maze Route cannot find path of net {} in worker of routeBox {}. |
| DRT | 0256 | Skipping NDR {} because another rule with the same name already exists. |
| DRT | 0258 | Unsupported LEF58_SPACING rule for layer {} of type AREA. |
| DRT | 0259 | Unsupported LEF58_SPACING rule for layer {} of type SAMEMASK. |
| DRT | 0260 | Unsupported LEF58_SPACING rule for layer {} of type PARALLELOVERLAP. |
| DRT | 0261 | Unsupported LEF58_SPACING rule for layer {} of type PARALLELWITHIN. |
| DRT | 0262 | Unsupported LEF58_SPACING rule for layer {} of type SAMEMETALSHAREDEDGE. |
| DRT | 0263 | Unsupported LEF58_SPACING rule for layer {}. |
| DRT | 0266 | Deprecated outputTA param in params file. |
| DRT | 0267 | cpu time = {:02}:{:02}:{:02}, elapsed time = {:02}:{:02}:{:02}, memory = {:.2f} (MB), peak = {:.2f} (MB) |
| DRT | 0268 | Done with {} horizontal wires in {} frboxes and {} vertical wires in {} frboxes. |
| DRT | 0269 | Unsupported endofline spacing rule. |
| DRT | 0270 | Unsupported endofline spacing rule. |
| DRT | 0271 | Unsupported endofline spacing rule. |
| DRT | 0272 | bottomRoutingLayer {} not found. |
| DRT | 0273 | topRoutingLayer {} not found. |
| DRT | 0274 | Deprecated threads param in params file. Use 'set_thread_count'. |
| DRT | 0275 | AP ({:.5f}, {:.5f}) (layer {}) (cost {}). |
| DRT | 0276 | Valid access pattern combination not found. |
| DRT | 0277 | Valid access pattern not found. |
| DRT | 0278 | Valid access pattern not found. |
| DRT | 0279 | SAMEMASK unsupported for cut LEF58_SPACINGTABLE rule |
| DRT | 0280 | Unknown type {} in setObjAP |
| DRT | 0281 | Marker {} at ({}, {}) ({}, {}). |
| DRT | 0282 | Skipping blockage. Cannot find layer {}. |
| DRT | 0290 | Warning: no DRC report specified, skipped writing DRC report |
| DRT | 0291 | Unexpected source type in marker: {} |
| DRT | 0292 | Marker {} at ({}, {}) ({}, {}). |
| DRT | 0293 | pin name {} has no ':' delimiter |
| DRT | 0294 | master {} not found in db |
| DRT | 0295 | mterm {} not found in db |
| DRT | 0296 | Mismatch in number of pins for term {}/{} |
| DRT | 0297 | inst {} not found in db |
| DRT | 0298 | iterm {} not found in db |
| DRT | 0299 | Mismatch in access points size {} and term pins size {} |
| DRT | 0300 | Preferred access point is not found |
| DRT | 0301 | bterm {} not found in db |
| DRT | 0302 | Unsupported multiple pins on bterm {} |
| DRT | 0303 | Mismatch in number of pins for bterm {} |
| DRT | 0304 | Updating design remotely failed |
| DRT | 0305 | Net {} of signal type {} is not routable by TritonRoute. Move to special nets. |
| DRT | 0306 | Net {} of signal type {} cannot be connected to bterm {} with signal type {} |
| DRT | 0307 | Net {} of signal type {} cannot be connected to iterm {}/{} with signal type {} |
| DRT | 0308 | step_dr requires nine positional arguments. |
| DRT | 0309 | Deprecated guide param in params file. use read_guide instead. |
| DRT | 0310 | Deprecated outputguide param in params file. use write_guide instead. |
| DRT | 0311 | Unsupported branch EXCEPTMINWIDTH in PROPERTY LEF58_AREA. |
| DRT | 0312 | Unsupported branch EXCEPTEDGELENGTH in PROPERTY LEF58_AREA. |
| DRT | 0313 | Unsupported branch EXCEPTMINSIZE in PROPERTY LEF58_AREA. |
| DRT | 0314 | Unsupported branch EXCEPTSTEP in PROPERTY LEF58_AREA. |
| DRT | 0315 | Unsupported branch MASK in PROPERTY LEF58_AREA. |
| DRT | 0316 | Unsupported branch LAYER in PROPERTY LEF58_AREA. |
| DRT | 0317 | LEF58_MINIMUMCUT AREA is not supported. Skipping for layer {} |
| DRT | 0318 | LEF58_MINIMUMCUT SAMEMETALOVERLAP is not supported. Skipping for layer {} |
| DRT | 0319 | LEF58_MINIMUMCUT FULLYENCLOSED is not supported. Skipping for layer {} |
| DRT | 0320 | Term {} of {} contains offgrid pin shape |
| DRT | 0321 | Term {} of {} contains offgrid pin shape |
| DRT | 0322 | checkFigsOnGrid unsupported pinFig. |
| DRT | 0323 | Via(s) in pin {} of {} will be ignored |
| DRT | 0324 | LEF58_KEEPOUTZONE SAMEMASK is not supported. Skipping for layer {} |
| DRT | 0325 | LEF58_KEEPOUTZONE SAMEMETAL is not supported. Skipping for layer {} |
| DRT | 0326 | LEF58_KEEPOUTZONE DIFFMETAL is not supported. Skipping for layer {} |
| DRT | 0327 | LEF58_KEEPOUTZONE EXTENSION is not supported. Skipping for layer {} |
| DRT | 0328 | LEF58_KEEPOUTZONE non zero SPIRALEXTENSION is not supported. Skipping for layer {} |
| DRT | 0329 | Error sending INST_ROWS Job to cloud |
| DRT | 0330 | Error sending UPDATE_PATTERNS Job to cloud |
| DRT | 0331 | Error sending UPDATE_PA Job to cloud |
| DRT | 0332 | Error sending UPDATE_PA Job to cloud |
| DRT | 0400 | Unsupported LEF58_SPACING rule with option EXCEPTEXACTWIDTH for layer {}. |
| DRT | 0401 | Unsupported LEF58_SPACING rule with option FILLCONCAVECORNER for layer {}. |
| DRT | 0403 | Unsupported LEF58_SPACING rule with option EQUALRECTWIDTH for layer {}. |
| DRT | 0404 | mterm {} not found in db |
| DRT | 0405 | Mismatch in number of pins for term {}/{} |
| DRT | 0406 | No {} tracks found in ({}, {}) for layer {} |
| DRT | 0410 | frNet not found. |
| DRT | 0411 | frNet {} does not have drNets. |
| DRT | 0412 | assignIroute_getDRCCost_helper overlap value is {}. |
| DRT | 0415 | Net {} already has routes. |
| DRT | 0416 | Term {} of {} contains offgrid pin shape. Pin shape {} is not a multiple of the manufacturing grid {}. |
| DRT | 0417 | Term {} of {} contains offgrid pin shape. Polygon point {} is not a multiple of the manufacturing grid {}. |
| DRT | 0500 | Sending worker {} failed |
| DRT | 0506 | -remote_host is required for distributed routing. |
| DRT | 0507 | -remote_port is required for distributed routing. |
| DRT | 0508 | -shared_volume is required for distributed routing. |
| DRT | 0512 | Unsupported region removeBlockObj |
| DRT | 0513 | Unsupported region addBlockObj |
| DRT | 0516 | -cloud_size is required for distributed routing. |
| DRT | 0517 | -dump_dir is required for detailed_route_run_worker command |
| DRT | 0519 | Via cut classes in LEF58_METALWIDTHVIAMAP are not supported. |
| DRT | 0520 | -worker_dir is required for detailed_route_run_worker command |
| DRT | 0550 | addToByte overflow |
| DRT | 0551 | subFromByte underflow |
| DRT | 0552 | -remote_host is required for distributed routing. |
| DRT | 0553 | -remote_port is required for distributed routing. |
| DRT | 0554 | -shared_volume is required for distributed routing. |
| DRT | 0555 | -cloud_size is required for distributed routing. |
| DRT | 0606 | via in pin bottom layer {} not found. |
| DRT | 0607 | via in pin top layer {} not found. |
| DRT | 0608 | Could not find user defined via {} |
| DRT | 0610 | Load design before setting default vias |
| DRT | 0611 | Via {} not found |
| DRT | 0612 | -box is a list of 4 coordinates. |
| DRT | 0613 | -output_file is required for check_drc command |
| DRT | 0615 | Load tech before setting unidirectional layers |
| DRT | 0616 | Layer {} not found |
| DRT | 0617 | PDN layer {} not found. |
| DRT | 0999 | Can't serialize used worker |
| DRT | 1000 | Pin {} not in any guide. Attempting to patch guides to cover (at least part of) the pin. |
| DRT | 1001 | No guide in the pin neighborhood |
| DRT | 1002 | Layer is not horizontal or vertical |
| DRT | 1003 | enclosesPlanarAccess: layer is neither vertical or horizontal |
| DRT | 1004 | enclosesPlanarAccess: low track not found |
| DRT | 1005 | enclosesPlanarAccess: high track not found |
| DRT | 1006 | failed to setTargetNet |
| DRT | 1007 | PatchGuides invoked with non-term object. |
| DRT | 1008 | checkPinForGuideEnclosure invoked with non-term object. |
| DRT | 1009 | initNet_term_new invoked with non-term object. |
| DRT | 1010 | Unsupported non-orthogonal wire begin=({}, {}) end=({}, {}), layer {} |
| DRT | 1011 | Access Point not found for iterm {}/{} |
| DRT | 12304 | Updating design remotely failed |
| DRT | 2000 | ({} {} {} coords: {} {} {}\n |
| DRT | 2001 | Starting worker ({} {}) ({} {}) with {} markers |
| DRT | 2002 | Routing net {} |
| DRT | 2003 | Ending net {} with markers: |
| DRT | 2005 | Creating dest search points from pins: |
| DRT | 2006 | Pin {} |
| DRT | 2007 | ({} {} {} coords: {} {} {}\n |
| DRT | 2008 | -dump_dir is required for debugging with -dump_dr. |
| DRT | 3000 | Guide in layer {} which is above max routing layer {} |
| DRT | 4000 | DEBUGGING inst {} term {} |
| DRT | 4500 | Edge outer dir should be either North or South |
| DRT | 4501 | Edge outer dir should be either East or West |
| DRT | 5000 | INST NOT FOUND! |
| DRT | 6000 | Macro pin has more than 1 polygon |
| DRT | 6001 | Path segs were not split: {} and {} |
| DRT | 7461 | Balancer failed |
| DRT | 9199 | Guide {} out of range {} |
| DRT | 9504 | Updating globals remotely failed |
| DRT | 9999 | unknown update type {} |
| DST | 0001 | Worker server error: {} |
| DST | 0002 | -host is required in run_worker cmd. |
| DST | 0003 | -port is required in run_worker cmd. |
| DST | 0004 | WorkerConnection.cc:122   Worker conhandler failed with message: \"{}\" |
| DST | 0005 | Unsupported job type {} from port {} |
| DST | 0006 | No workers available |
| DST | 0007 | Processed {} jobs |
| DST | 0008 | BalancerConnection.cc:231 Balancer conhandler failed with message: {} |
| DST | 0009 | LoadBalancer error: {} |
| DST | 0010 | -host is required in run_load_balancer cmd. |
| DST | 0011 | -port is required in run_load_balancer cmd. |
| DST | 0012 | Serializing JobMessage failed |
| DST | 0013 | Socket connection failed with message \"{}\" |
| DST | 0014 | Sending job failed with message \"{}\" |
| DST | 0016 | -host is required in add_worker_address cmd. |
| DST | 0017 | -port is required in add_worker_address cmd. |
| DST | 0020 | Serializing result JobMessage failed |
| DST | 0022 | Sending result failed with message \"{}\" |
| DST | 0041 | Received malformed msg {} from port {} |
| DST | 0042 | Received malformed msg {} from port {} |
| DST | 0112 | Serializing JobMessage failed |
| DST | 0113 | Trial {}, socket connection failed with message \"{}\" |
| DST | 0114 | Sending job failed with message \"{}\" |
| DST | 0203 | Workers domain resolution failed with error code = {}. Message = {}. |
| DST | 0204 | Exception thrown: {}. worker with ip \"{}\" and port \"{}\" will be pushed back the queue. |
| DST | 0205 | Maximum of {} failing workers reached, relaying error to leader. |
| DST | 0207 | {} workers failed to receive the broadcast message and have been removed. |
| DST | 9999 | Problem in deserialize {} |
| FIN | 0001 | Layer {} in names was not found. |
| FIN | 0002 | Layer {} not found. |
| FIN | 0003 | Filling layer {}. |
| FIN | 0004 | Total fills: {}. |
| FIN | 0005 | Filling {} areas with OPC fill. |
| FIN | 0006 | Total fills: {}. |
| FIN | 0007 | The -rules argument must be specified. |
| FIN | 0008 | The -area argument must be a list of 4 coordinates. |
| FIN | 0009 | Filling {} areas with non-OPC fill. |
| FIN | 0010 | Skipping layer {}. |
| GPL | 0002 | DBU: {} |
| GPL | 0003 | SiteSize: {} {} |
| GPL | 0004 | CoreAreaLxLy: {} {} |
| GPL | 0005 | CoreAreaUxUy: {} {} |
| GPL | 0006 | NumInstances: {} |
| GPL | 0007 | NumPlaceInstances: {} |
| GPL | 0008 | NumFixedInstances: {} |
| GPL | 0009 | NumDummyInstances: {} |
| GPL | 0010 | NumNets: {} |
| GPL | 0011 | NumPins: {} |
| GPL | 0012 | DieAreaLxLy: {} {} |
| GPL | 0013 | DieAreaUxUy: {} {} |
| GPL | 0014 | CoreAreaLxLy: {} {} |
| GPL | 0015 | CoreAreaUxUy: {} {} |
| GPL | 0016 | CoreArea: {} |
| GPL | 0017 | NonPlaceInstsArea: {} |
| GPL | 0018 | PlaceInstsArea: {} |
| GPL | 0019 | Util(%): {:.2f} |
| GPL | 0020 | StdInstsArea: {} |
| GPL | 0021 | MacroInstsArea: {} |
| GPL | 0023 | TargetDensity: {:.2f} |
| GPL | 0024 | AveragePlaceInstArea: {} |
| GPL | 0025 | IdealBinArea: {} |
| GPL | 0026 | IdealBinCnt: {} |
| GPL | 0027 | TotalBinArea: {} |
| GPL | 0028 | BinCnt: {} {} |
| GPL | 0029 | BinSize: {} {} |
| GPL | 0030 | NumBins: {} |
| GPL | 0031 | FillerInit: NumGCells: {} |
| GPL | 0032 | FillerInit: NumGNets: {} |
| GPL | 0033 | FillerInit: NumGPins: {} |
| GPL | 0034 | gCellFiller: {} |
| GPL | 0035 | NewTotalFillerArea: {} |
| GPL | 0036 | TileLxLy: {} {} |
| GPL | 0037 | TileSize: {} {} |
| GPL | 0038 | TileCnt: {} {} |
| GPL | 0039 | numRoutingLayers: {} |
| GPL | 0040 | NumTiles: {} |
| GPL | 0045 | InflatedAreaDelta: {} |
| GPL | 0046 | TargetDensity: {} |
| GPL | 0047 | SavedMinRC: {} |
| GPL | 0048 | SavedTargetDensity: {} |
| GPL | 0049 | WhiteSpaceArea: {} |
| GPL | 0050 | NesterovInstsArea: {} |
| GPL | 0051 | TotalFillerArea: {} |
| GPL | 0052 | TotalGCellsArea: {} |
| GPL | 0053 | ExpectedTotalGCellsArea: {} |
| GPL | 0054 | NewTargetDensity: {} |
| GPL | 0055 | NewWhiteSpaceArea: {} |
| GPL | 0056 | MovableArea: {} |
| GPL | 0057 | NewNesterovInstsArea: {} |
| GPL | 0058 | NewTotalFillerArea: {} |
| GPL | 0059 | NewTotalGCellsArea: {} |
| GPL | 0063 | TotalRouteOverflowH2: {} |
| GPL | 0064 | TotalRouteOverflowV2: {} |
| GPL | 0065 | OverflowTileCnt2: {} |
| GPL | 0066 | 0.5%RC: {} |
| GPL | 0067 | 1.0%RC: {} |
| GPL | 0068 | 2.0%RC: {} |
| GPL | 0069 | 5.0%RC: {} |
| GPL | 0070 | 0.5rcK: {} |
| GPL | 0071 | 1.0rcK: {} |
| GPL | 0072 | 2.0rcK: {} |
| GPL | 0073 | 5.0rcK: {} |
| GPL | 0074 | FinalRC: {} |
| GPL | 0075 | Routability numCall: {} inflationIterCnt: {} bloatIterCnt: {} |
| GPL | 0100 | worst slack {:.3g} |
| GPL | 0102 | No slacks found. Timing-driven mode disabled. |
| GPL | 0103 | Weighted {} nets. |
| GPL | 0114 | No net slacks found. Timing-driven mode disabled. |
| GPL | 0115 | -disable_timing_driven is deprecated. |
| GPL | 0116 | -disable_routability_driven is deprecated. |
| GPL | 0118 | core area outside of die. |
| GPL | 0119 | instance {} height is larger than core. |
| GPL | 0120 | instance {} width is larger than core. |
| GPL | 0121 | No liberty libraries found. |
| GPL | 0130 | No rows defined in design. Use initialize_floorplan to add rows. |
| GPL | 0131 | No rows defined in design. Use initialize_floorplan to add rows. |
| GPL | 0132 | Locked {} instances |
| GPL | 0133 | Unlocked instances |
| GPL | 0134 | Master {} is not marked as a BLOCK in LEF but is more than {} rows tall.  It will be treated as a macro. |
| GPL | 0135 | Target density must be in \[0, 1\]. |
| GPL | 0136 | No placeable instances - skipping placement. |
| GPL | 0150 | -skip_io will disable timing driven mode. |
| GPL | 0151 | -skip_io will disable routability driven mode. |
| GPL | 0250 | GPU is not available. CPU solve is being used. |
| GPL | 0251 | CPU solver is forced to be used. |
| GPL | 0301 | Utilization exceeds 100%. |
| GPL | 0302 | Use a higher -density or re-floorplan with a larger core area.\nGiven target density: {:.2f}\nSuggested target density: {:.2f} |
| GPL | 0303 | Use a higher -density or re-floorplan with a larger core area.\nGiven target density: {:.2f}\nSuggested target density: {:.2f} |
| GPL | 0304 | RePlAce diverged at initial iteration with steplength being {}. Re-run with a smaller init_density_penalty value. |
| GPL | 0305 | Unable to find a site |
| GRT | 0001 | Minimum degree: {} |
| GRT | 0002 | Maximum degree: {} |
| GRT | 0003 | Macros: {} |
| GRT | 0004 | Blockages: {} |
| GRT | 0005 | Layer $layer_name not found. |
| GRT | 0006 | Repairing antennas, iteration {}. |
| GRT | 0009 | rerouting {} nets. |
| GRT | 0012 | Found {} antenna violations. |
| GRT | 0014 | Routed nets: {} |
| GRT | 0015 | Inserted {} diodes. |
| GRT | 0018 | Total wirelength: {} um |
| GRT | 0019 | Found {} clock nets. |
| GRT | 0020 | Min routing layer: {} |
| GRT | 0021 | Max routing layer: {} |
| GRT | 0022 | GlobalRouter.cpp:3534     Global adjustment: {}% |
| GRT | 0023 | Grid origin: ({}, {}) |
| GRT | 0025 | Non wire or via route found on net {}. |
| GRT | 0026 | Missing route to pin {}. |
| GRT | 0027 | Design has rows with different site widths. |
| GRT | 0028 | Found {} pins outside die area. |
| GRT | 0029 | Pin {} does not have geometries below the max routing layer ({}). |
| GRT | 0030 | Specified layer {} for adjustment is greater than max routing layer {} and will be ignored. |
| GRT | 0031 | At least 2 pins in position ({}, {}), layer {}, port {}. |
| GRT | 0033 | Pin {} has invalid edge. |
| GRT | 0034 | Net connected to instance of class COVER added for routing. |
| GRT | 0035 | Pin {} is outside die area. |
| GRT | 0036 | Pin {} is outside die area. |
| GRT | 0037 | Found blockage outside die area. |
| GRT | 0038 | Found blockage outside die area in instance {}. |
| GRT | 0039 | Found pin {} outside die area in instance {}. |
| GRT | 0040 | Net {} has wires outside die area. |
| GRT | 0041 | Net {} has wires outside die area. |
| GRT | 0042 | Pin {} does not have geometries in a valid routing layer. |
| GRT | 0043 | No OR_DEFAULT vias defined. |
| GRT | 0044 | set_global_routing_layer_adjustment requires layer and adj arguments. |
| GRT | 0045 | Run global_route before repair_antennas. |
| GRT | 0047 | Missing dbTech. |
| GRT | 0048 | Command set_global_routing_region_adjustment is missing -layer argument. |
| GRT | 0049 | Command set_global_routing_region_adjustment is missing -adjustment argument. |
| GRT | 0050 | Command set_global_routing_region_adjustment needs four arguments to define a region: lower_x lower_y upper_x upper_y. |
| GRT | 0051 | Missing dbTech. |
| GRT | 0052 | Missing dbBlock. |
| GRT | 0053 | Routing resources analysis: |
| GRT | 0054 | Using detailed placer to place {} diodes. |
| GRT | 0055 | Wrong number of arguments for origin. |
| GRT | 0056 | In argument -clock_layers, min routing layer is greater than max routing layer. |
| GRT | 0057 | Missing track structure for layer $layer_name. |
| GRT | 0059 | Missing technology file. |
| GRT | 0060 | Layer [$tech_layer getConstName] is greater than the max routing layer ([$max_tech_layer getConstName]). |
| GRT | 0061 | Layer [$tech_layer getConstName] is less than the min routing layer ([$min_tech_layer getConstName]). |
| GRT | 0062 | Input format to define layer range for $cmd is min-max. |
| GRT | 0063 | Missing dbBlock. |
| GRT | 0064 | Lower left x is outside die area. |
| GRT | 0065 | Lower left y is outside die area. |
| GRT | 0066 | Upper right x is outside die area. |
| GRT | 0067 | Upper right y is outside die area. |
| GRT | 0068 | Global route segment not valid. |
| GRT | 0069 | Diode cell $diode_cell not found. |
| GRT | 0071 | Layer spacing not found. |
| GRT | 0072 | Informed region is outside die area. |
| GRT | 0073 | Diode cell has more than one non power/ground port. |
| GRT | 0074 | Routing with guides in blocked metal for net {}. |
| GRT | 0075 | Connection between non-adjacent layers in net {}. |
| GRT | 0076 | Net {} not properly covered. |
| GRT | 0077 | Segment has invalid layer assignment. |
| GRT | 0078 | Guides vector is empty. |
| GRT | 0079 | Pin {} does not have layer assignment. |
| GRT | 0080 | Invalid pin placement. |
| GRT | 0082 | Cannot find track spacing. |
| GRT | 0084 | Layer {} does not have a valid direction. |
| GRT | 0085 | Routing layer {} not found. |
| GRT | 0086 | Track for layer {} not found. |
| GRT | 0088 | Layer {:7s} Track-Pitch = {:.4f}  line-2-Via Pitch: {:.4f} |
| GRT | 0090 | Track for layer {} not found. |
| GRT | 0094 | Design with no nets. |
| GRT | 0096 | Final congestion report: |
| GRT | 0101 | Running extra iterations to remove overflow. |
| GRT | 0103 | Extra Run for hard benchmark. |
| GRT | 0111 | Final number of vias: {} |
| GRT | 0112 | Final usage 3D: {} |
| GRT | 0113 | Underflow in reduce: cap, reducedCap: {}, {} |
| GRT | 0114 | Underflow in reduce: cap, reducedCap: {}, {} |
| GRT | 0115 | GlobalRouter.cpp:340      Global routing finished with overflow. |
| GRT | 0118 | Routing congestion too high. Check the congestion heatmap in the GUI. |
| GRT | 0119 | Routing congestion too high. Check the congestion heatmap in the GUI and load {} in the DRC viewer. |
| GRT | 0121 | Route type is not maze, netID {}. |
| GRT | 0122 | Maze ripup wrong for net {}. |
| GRT | 0123 | Maze ripup wrong in newRipupNet for net {}. |
| GRT | 0124 | Horizontal tracks for layer {} not found. |
| GRT | 0125 | Setup heap: not maze routing. |
| GRT | 0146 | Argument -allow_overflow is deprecated. Use -allow_congestion. |
| GRT | 0147 | Vertical tracks for layer {} not found. |
| GRT | 0148 | Layer {} has invalid direction. |
| GRT | 0150 | Net {} has errors during updateRouteType1. |
| GRT | 0151 | Net {} has errors during updateRouteType2. |
| GRT | 0152 | Net {} has errors during updateRouteType1. |
| GRT | 0153 | Net {} has errors during updateRouteType2. |
| GRT | 0164 | Initial grid wrong y1 x1 [{} {}], net start [{} {}] routelen {}. |
| GRT | 0165 | End grid wrong y2 x2 [{} {}], net start [{} {}] routelen {}. |
| GRT | 0166 | Net {} edge[{}] maze route wrong, distance {}, i {}. |
| GRT | 0167 | Invalid 2D tree for net {}. |
| GRT | 0169 | Net {}: Invalid index for position ({}, {}). Net degree: {}. |
| GRT | 0170 | Net {}: Invalid index for position ({}, {}). Net degree: {}. |
| GRT | 0171 | Invalid index for position ({}, {}). |
| GRT | 0172 | Invalid index for position ({}, {}). |
| GRT | 0179 | Wrong node status {}. |
| GRT | 0181 | Wrong node status {}. |
| GRT | 0183 | Net {}: heap underflow during 3D maze routing. |
| GRT | 0184 | Shift to 0 length edge, type2. |
| GRT | 0187 | In 3D maze routing, type 1 node shift, cnt_n1A1 is 1. |
| GRT | 0188 | Invalid number of node neighbors. |
| GRT | 0189 | Failure in copy tree. Number of edges: {}. Number of nodes: {}. |
| GRT | 0197 | Via related to pin nodes: {} |
| GRT | 0198 | Via related Steiner nodes: {} |
| GRT | 0199 | Via filling finished. |
| GRT | 0200 | Start point not assigned. |
| GRT | 0201 | Setup heap: not maze routing. |
| GRT | 0202 | Target ending layer ({}) out of range. |
| GRT | 0203 | Caused floating pin node. |
| GRT | 0204 | Invalid layer value in gridsL, {}. |
| GRT | 0206 | Trying to recover a 0-length edge. |
| GRT | 0207 | Ripped up edge without edge length reassignment. |
| GRT | 0208 | Route length {}, tree length {}. |
| GRT | 0209 | Pin {} is completely outside the die area and cannot bet routed. |
| GRT | 0214 | Cannot get edge capacity: edge is not vertical or horizontal. |
| GRT | 0215 | -ratio_margin must be between 0 and 100 percent. |
| GRT | 0219 | Command set_macro_extension needs one argument: extension. |
| GRT | 0220 | Command set_pin_offset needs one argument: offset. |
| GRT | 0221 | Cannot create wire for net {}. |
| GRT | 0222 | No technology has been read. |
| GRT | 0223 | Missing dbBlock. |
| GRT | 0224 | Missing dbBlock. |
| GRT | 0225 | Maze ripup wrong in newRipup. |
| GRT | 0226 | Type2 ripup not type L. |
| GRT | 0228 | Horizontal edge usage exceeds the maximum allowed. ({}, {}) usage={} limit={} |
| GRT | 0229 | Vertical edge usage exceeds the maximum allowed. ({}, {}) usage={} limit={} |
| GRT | 0230 | Congestion iterations cannot increase overflow, reached the maximum number of times the total overflow can be increased. |
| GRT | 0231 | Net name not found. |
| GRT | 0232 | Routing congestion too high. Check the congestion heatmap in the GUI. |
| GRT | 0233 | Failed to open guide file {}. |
| GRT | 0234 | Cannot find net {}. |
| GRT | 0235 | Cannot find layer {}. |
| GRT | 0236 | Error reading guide file {}. |
| GRT | 0237 | Net {} global route wire length: {:.2f}um |
| GRT | 0238 | -net is required. |
| GRT | 0239 | Net {} does not have detailed route. |
| GRT | 0240 | Net {} detailed route wire length: {:.2f}um |
| GRT | 0241 | Net {} does not have global route. |
| GRT | 0242 | -seed argument is required. |
| GRT | 0243 | Unable to repair antennas on net with diodes. |
| GRT | 0244 | Diode {}/{} ANTENNADIFFAREA is zero. |
| GRT | 0245 | Too arguments to repair_antennas. |
| GRT | 0246 | No diode with LEF class CORE ANTENNACELL found. |
| GRT | 0247 | v_edge mismatch {} vs {} |
| GRT | 0248 | h_edge mismatch {} vs {} |
| GRT | 0249 | Load design before reading guides |
| GRT | 0250 | Net {} has guides but is not routed by the global router and will be skipped. |
| GRT | 0251 | The start_incremental and end_incremental flags cannot be defined together |
| GRT | 1247 | v_edge mismatch {} vs {} |
| GRT | 1248 | h_edge mismatch {} vs {} |
| GUI | 0001 | Command {} is not usable in non-GUI mode |
| GUI | 0002 | No database loaded |
| GUI | 0003 | No database loaded |
| GUI | 0005 | No chip loaded |
| GUI | 0006 | No block loaded |
| GUI | 0007 | Unknown display control type: {} |
| GUI | 0008 | GUI 0008 gui.cpp:1095              GUI already active. |
| GUI | 0009 | Unknown display control type: {} |
| GUI | 0010 | File path does not end with a valid extension, new path is: {} |
| GUI | 0011 | Failed to write image: {} |
| GUI | 0012 | Image size is not valid: {}px x {}px |
| GUI | 0013 | Unable to find {} display control at {}. |
| GUI | 0014 | Unable to find {} display control at {}. |
| GUI | 0015 | No design loaded. |
| GUI | 0016 | No design loaded. |
| GUI | 0017 | No technology loaded. |
| GUI | 0018 | Area must contain 4 elements. |
| GUI | 0019 | Display option must have 2 elements {control name} {value}. |
| GUI | 0020 | The -text argument must be specified. |
| GUI | 0021 | The -script argument must be specified. |
| GUI | 0022 | Button {} already defined. |
| GUI | 0023 | Failed to open help automatically, navigate to: {} |
| GUI | 0024 | Ruler with name \"{}\" already exists |
| GUI | 0025 | Menu action {} already defined. |
| GUI | 0026 | The -text argument must be specified. |
| GUI | 0027 | The -script argument must be specified. |
| GUI | 0028 | {} is not a known map. Valid options are: {} |
| GUI | 0029 | {} is not a valid option. Valid options are: {} |
| GUI | 0030 | Unable to open TritonRoute DRC report: {} |
| GUI | 0031 | Resolution too high for design, defaulting to [expr $resolution / [$tech getLefUnits]] um per pixel |
| GUI | 0032 | Unable to determine type of {} |
| GUI | 0033 | No descriptor is registered for {}. |
| GUI | 0034 | Found {} controls matching {} at {}. |
| GUI | 0035 | Unable to find descriptor for: {} |
| GUI | 0036 | Nothing selected |
| GUI | 0037 | Unknown property: {} |
| GUI | 0038 | Must specify -type. |
| GUI | 0039 | Cannot use case insensitivity without a name. |
| GUI | 0040 | Unable to find tech layer (line: {}): {} |
| GUI | 0041 | Unable to find bterm (line: {}): {} |
| GUI | 0042 | Unable to find iterm (line: {}): {} |
| GUI | 0043 | Unable to find instance (line: {}): {} |
| GUI | 0044 | Unable to find net (line: {}): {} |
| GUI | 0045 | Unable to parse line as violation type (line: {}): {} |
| GUI | 0046 | Unable to parse line as violation source (line: {}): {} |
| GUI | 0047 | Unable to parse line as violation location (line: {}): {} |
| GUI | 0048 | Unable to parse bounding box (line: {}): {} |
| GUI | 0049 | Unable to parse bounding box (line: {}): {} |
| GUI | 0050 | Unable to parse bounding box (line: {}): {} |
| GUI | 0051 | Unknown source type (line: {}): {} |
| GUI | 0052 | Unable to find obstruction (line: {}) |
| GUI | 0053 | Unable to find descriptor for: {} |
| GUI | 0054 | Unknown data type for \"{}\". |
| GUI | 0055 | Unable to parse JSON file {}: {} |
| GUI | 0056 | Unable to parse violation shape: {} |
| GUI | 0057 | Unknown data type \"{}\" for \"{}\". |
| GUI | 0060 | {} must be a boolean |
| GUI | 0061 | {} must be an integer or double |
| GUI | 0062 | {} must be an integer or double |
| GUI | 0063 | {} must be a string |
| GUI | 0064 | No design loaded. |
| GUI | 0065 | No design loaded. |
| GUI | 0066 | Heat map \"{}\" has not been populated with data. |
| GUI | 0067 | Design not loaded. |
| GUI | 0068 | Pin not found. |
| GUI | 0069 | Multiple pin timing cones are not supported. |
| GUI | 0070 | Design not loaded. |
| GUI | 0071 | Unable to find net \"$net_name\". |
| GUI | 0072 | \"{}\" is not populated with data. |
| GUI | 0073 | Unable to open {} |
| GUI | 0074 | Unable to find clock: {} |
| GUI | 0076 | Unknown filetype: {} |
| GUI | 0077 | Timing data is not stored in {} and must be loaded separately, if needed. |
| GUI | 0078 | Failed to write image: {} |
| IFP | 0001 | Added {} rows of {} site {} with height {}. |
| IFP | 0010 | layer $layer_name not found. |
| IFP | 0011 | Unable to find site: $sitename |
| IFP | 0013 | -core_space is either a list of 4 margins or one value for all margins. |
| IFP | 0015 | -die_area is a list of 4 coordinates. |
| IFP | 0016 | -core_area is a list of 4 coordinates. |
| IFP | 0017 | no -core_area specified. |
| IFP | 0019 | no -utilization or -die_area specified. |
| IFP | 0021 | Track pattern for {} will be skipped due to x_offset > die width. |
| IFP | 0022 | Track pattern for {} will be skipped due to y_offset > die height. |
| IFP | 0025 | layer $layer_name is not a routing layer. |
| IFP | 0026 | left core row: {} has less than 10 sites |
| IFP | 0027 | right core row: {} has less than 10 sites |
| IFP | 0028 | Core area lower left ({:.3f}, {:.3f}) snapped to ({:.3f}, {:.3f}). |
| IFP | 0029 | Unable to determine tiecell ({}) function. |
| IFP | 0030 | Inserted {} tiecells using {}/{}. |
| IFP | 0031 | Unable to find master: $tie_cell |
| IFP | 0032 | Unable to find master pin: $args |
| IFP | 0038 | No design is loaded. |
| IFP | 0039 | Liberty cell or port {}/{} not found. |
| IFP | 0040 | Invalid height for site {} detected. The height value of {} is not a multiple of the smallest site height {}. |
| IFP | 0042 | No sites found. |
| IFP | 0043 | No site found for instance {} in block {}. |
| IFP | 0044 | Non horizontal layer uses property LEF58_PITCH. |
| IFP | 0045 | No routing Row found in layer {} |
| MPL | 0001 | No block found for Macro Placement. |
| MPL | 0002 | Some instances do not have Liberty models. TritonMP will place macros without connection information. |
| MPL | 0002 | Floorplan has not been initialized? Pin location error for {}. |
| MPL | 0003 | MacroPlacer.cpp:808       Macro {} is unplaced, use global_placement to get an initial placement before macro placement. |
| MPL | 0003 | There are no valid tilings for mixed cluster: {} |
| MPL | 0004 | No macros found. |
| MPL | 0004 | This no valid tilings for hard macro cluser: {} |
| MPL | 0005 | Found {} macros. |
| MPL | 0005 | [MultiLevelMacroPlacement] Failed on cluster: {} |
| MPL | 0006 | SA failed on cluster: {} |
| MPL | 0007 | Not enough space in cluster: {} for child hard macro cluster: {} |
| MPL | 0008 | Not enough space in cluster: {} for child mixed cluster: {} |
| MPL | 0009 | {} pins {}. |
| MPL | 0009 | Fail !!! Bus planning error !!! |
| MPL | 0010 | Cannot find valid macro placement for hard macro cluster: {} |
| MPL | 0011 | Pin {} is not placed, using west. |
| MPL | 0012 | Unhandled partition class. |
| MPL | 0013 | bus_synthesis.cpp:1117    bus planning - error condition nullptr |
| MPL | 0014 | bus_synthesis.cpp:1131    bus planning - error condition pre_cluster |
| MPL | 0015 | Unexpected orientation |
| MPL | 0061 | Parquet area {:g} x {:g} exceeds the partition area {:g} x {:g}. |
| MPL | 0066 | Partitioning failed. |
| MPL | 0067 | Initial weighted wire length {:g}. |
| MPL | 0068 | Placed weighted wire length {:g}. |
| MPL | 0069 | Initial weighted wire length {:g}. |
| MPL | 0070 | Using {} partition sets. |
| MPL | 0071 | Solution {} weighted wire length {:g}. |
| MPL | 0072 | No partition solutions found. |
| MPL | 0073 | Best weighted wire length {:g}. |
| MPL | 0076 | Partition {} macros. |
| MPL | 0077 | Using {} cut lines. |
| MPL | 0079 | Cut line {:.2f}. |
| MPL | 0080 | Impossible partition found. |
| MPL | 0085 | fence_region outside of core area. Using core area. |
| MPL | 0089 | No rows found. Use initialize_floorplan to add rows. |
| MPL | 0092 | -halo receives a list with 2 values, [llength $halo] given. |
| MPL | 0093 | -channel receives a list with 2 values, [llength $channel] given. |
| MPL | 0094 | -fence_region receives a list with 4 values, [llength $fence_region] given. |
| MPL | 0095 | Snap layer $snap_layer is not a routing layer. |
| MPL | 0096 | Unknown placement style. Use one of corner_max_wl or corner_min_wl. |
| MPL | 0097 | Unable to find a site |
| MPL | 2067 | [MultiLevelMacroPlacement] Failed on cluster {} |
| ODB | 0000 | {} Net {} not found |
| ODB | 0002 | Error opening file {} |
| ODB | 0004 | Hierarchical block information is lost |
| ODB | 0005 | Can not find master {} |
| ODB | 0006 | Can not find net {} |
| ODB | 0007 | Can not find inst {} |
| ODB | 0008 | Cannot duplicate net {} |
| ODB | 0009 | id mismatch ({},{}) for net {} |
| ODB | 0010 | tot = {}, upd = {}, enc = {} |
| ODB | 0011 | net {} {} capNode {} ccseg {} has otherCapNode {} not from changed or halo nets |
| ODB | 0012 | the other capNode is from net {} {} |
| ODB | 0013 | ccseg {} has other capn {} not from changed or halo nets |
| ODB | 0014 | Failed to generate non-default rule for single wire net {} |
| ODB | 0015 | disconnected net {} |
| ODB | 0016 | problem in getExtension() |
| ODB | 0017 | Failed to write def file {} |
| ODB | 0018 | error in addToWire |
| ODB | 0019 | Can not open file {} to write! |
| ODB | 0020 | Memory allocation failed for io buffer |
| ODB | 0021 | ccSeg={} capn0={} next0={} capn1={} next1={} |
| ODB | 0022 | ccSeg {} has capnd {} {}, not {} ! |
| ODB | 0023 | CCSeg.cpp:755           CCSeg {} does not have orig capNode {}. Can not swap. |
| ODB | 0024 | cc seg {} has both capNodes {} {} from the same net {} . ignored by groundCC . |
| ODB | 0025 | capn {} |
| ODB | 0034 | Can not find physical location of iterm {}/{} |
| ODB | 0036 | setLevel {} greater than 255 is illegal! inst {} |
| ODB | 0037 | instance bound to a block {} |
| ODB | 0038 | instance already bound to a block {} |
| ODB | 0039 | Forced Initialize to 0 |
| ODB | 0040 | block already bound to an instance {} |
| ODB | 0041 | Forced Initialize to 0 |
| ODB | 0042 | block not a direct child of instance owner |
| ODB | 0043 | _dbHier::create fails |
| ODB | 0044 | Failed(_hierarchy) to swap: {} -> {} {} |
| ODB | 0045 | Failed(termSize) to swap: {} -> {} {} |
| ODB | 0046 | Failed(mtermEquiv) to swap: {} -> {} {} |
| ODB | 0047 | instance {} has no output pin |
| ODB | 0048 | Net.cpp:1013            Net {} {} had been CC adjusted by {}. Please unadjust first. |
| ODB | 0049 | Donor net {} {} has no rc data |
| ODB | 0050 | Donor net {} {} has no capnode attached to iterm {}/{} |
| ODB | 0051 | Receiver net {} {} has no capnode attached to iterm {}/{} |
| ODB | 0052 | Net.cpp:2541            Net {}, {} has no extraction data |
| ODB | 0053 | Net.cpp:2587            Net {}, {} has no extraction data |
| ODB | 0054 | CC segs of RSeg {}-{} |
| ODB | 0055 | CC{} : {}-{} |
| ODB | 0056 | rseg {} |
| ODB | 0057 | Cannot find cap nodes for Rseg {} |
| ODB | 0058 | Layer {} is not a routing layer! |
| ODB | 0059 | Layer {}, routing level {}, has {} pitch !! |
| ODB | 0060 | Layer {}, routing level {}, has {} width !! |
| ODB | 0061 | Layer {}, routing level {}, has {} spacing !! |
| ODB | 0062 | This wire has no net |
| ODB | 0063 | {} No wires for net {} |
| ODB | 0064 | {} begin decoder for net {} |
| ODB | 0065 | {} End decoder for net {} |
| ODB | 0066 | {} New path: layer {} type {}  non-default rule {} |
| ODB | 0067 | {} New path: layer {} type {}\n |
| ODB | 0068 | {} New path at junction {}, point(ext) {} {} {}, with rule {} |
| ODB | 0069 | {} New path at junction {}, point(ext) {} {} {} |
| ODB | 0070 | {} New path at junction {}, point {} {}, with rule {} |
| ODB | 0071 | {} New path at junction {}, point {} {} |
| ODB | 0072 | {} opcode after junction is not point or point_ext??\n |
| ODB | 0073 | {} Short at junction {}, with rule {} |
| ODB | 0074 | {} Short at junction {} |
| ODB | 0075 | {} Virtual wire at junction {}, with rule {} |
| ODB | 0076 | {} Virtual wire at junction {} |
| ODB | 0077 | {} Found point {} {} |
| ODB | 0078 | {} Found point(ext){} {} {} |
| ODB | 0079 | {} Found via {} |
| ODB | 0080 | block via found in signal net! |
| ODB | 0081 | {} Found Iterm |
| ODB | 0082 | {} Found Bterm |
| ODB | 0083 | {} GOT RULE {}, EXPECTED RULE {} |
| ODB | 0084 | {} Found Rule {} in middle of path |
| ODB | 0085 | {} End decoder for net {} |
| ODB | 0086 | {} Hit default! |
| ODB | 0087 | {} No wires for net {} |
| ODB | 0088 | error: undefined layer ({}) referenced |
| ODB | 0089 | error: undefined component ({}) referenced |
| ODB | 0090 | error: undefined component ({}) referenced |
| ODB | 0091 | warning: Blockage max density {} not in [0, 100] will be ignored |
| ODB | 0092 | error: unknown library cell referenced ({}) for instance ({}) |
| ODB | 0093 | error: duplicate instance definition({}) |
| ODB | 0094 | \t\tCreated {} Insts |
| ODB | 0095 | error: undefined layer ({}) referenced |
| ODB | 0096 | net {} does not exist |
| ODB | 0097 | \t\tCreated {} Nets |
| ODB | 0098 | duplicate must-join net found ({}) |
| ODB | 0099 | error: netlist component ({}) is not defined |
| ODB | 0100 | error: netlist component-pin ({}, {}) is not defined |
| ODB | 0101 | error: undefined NONDEFAULTRULE ({}) referenced |
| ODB | 0102 | error: NONDEFAULTRULE ({}) of net ({}) does not match DEF rule ({}). |
| ODB | 0103 | error: undefined NONDEFAULTRULE ({}) referenced |
| ODB | 0104 | error: undefined layer ({}) referenced |
| ODB | 0105 | error: RULE ({}) referenced for layer ({}) |
| ODB | 0106 | error: undefined TAPER RULE ({}) referenced |
| ODB | 0107 | error: undefined via ({}) referenced |
| ODB | 0108 | error: invalid VIA layers, cannot determine exit layer of path |
| ODB | 0109 | error: undefined via ({}) referenced |
| ODB | 0110 | error: invalid VIA layers in {} in net {}, currently on layer {} at ({}, {}), cannot determine exit layer of path |
| ODB | 0111 | error: Duplicate NONDEFAULTRULE {} |
| ODB | 0112 | error: Cannot find tech-via {} |
| ODB | 0113 | error: Cannot find tech-via-generate rule {} |
| ODB | 0114 | error: Cannot find layer {} |
| ODB | 0115 | error: Cannot find layer {} |
| ODB | 0116 | error: Duplicate layer rule ({}) in non-default-rule statement. |
| ODB | 0117 | PIN {} missing right bus character. |
| ODB | 0118 | PIN {} missing left bus character. |
| ODB | 0119 | error: Cannot specify effective width and minimum spacing together. |
| ODB | 0120 | error: Cannot specify effective width and minimum spacing together. |
| ODB | 0121 | error: undefined layer ({}) referenced |
| ODB | 0122 | error: Cannot find PIN {} |
| ODB | 0123 | error: Cannot find PIN {} |
| ODB | 0124 | warning: Polygon DIEAREA statement not supported.  The bounding box will be used instead |
| ODB | 0125 | lines processed: {} |
| ODB | 0126 | error: {} |
| ODB | 0127 | Reading DEF file: {} |
| ODB | 0128 | Design: {} |
| ODB | 0129 | Error: Failed to read DEF file |
| ODB | 0130 | Created {} pins. |
| ODB | 0131 | Created {} components and {} component-terminals. |
| ODB | 0132 | Created {} special nets and {} connections. |
| ODB | 0133 | Created {} nets and {} connections. |
| ODB | 0134 | Finished DEF file: {} |
| ODB | 0135 | Reading DEF file: {} |
| ODB | 0137 | Error: Failed to read DEF file |
| ODB | 0138 | Created {} pins. |
| ODB | 0139 | Created {} components and {} component-terminals. |
| ODB | 0140 | Created {} special nets and {} connections. |
| ODB | 0141 | Created {} nets and {} connections. |
| ODB | 0142 | Finished DEF file: {} |
| ODB | 0143 | Reading DEF file: {} |
| ODB | 0144 | Error: Failed to read DEF file |
| ODB | 0145 | Processed {} special nets. |
| ODB | 0146 | Processed {} nets. |
| ODB | 0147 | Finished DEF file: {} |
| ODB | 0148 | error: Cannot open DEF file {} |
| ODB | 0149 | DEF parser returns an error! |
| ODB | 0150 | error: Cannot open DEF file {} |
| ODB | 0151 | DEF parser returns an error! |
| ODB | 0152 | Region.cpp:65        Region \"{}\" already exists |
| ODB | 0155 | error: undefined site ({}) referenced in row ({}) statement. |
| ODB | 0156 | special net {} does not exist |
| ODB | 0157 | error: netlist component ({}) is not defined |
| ODB | 0158 | error: netlist component-pin ({}, {}) is not defined |
| ODB | 0159 | error: undefined layer ({}) referenced |
| ODB | 0160 | error: undefined layer ({}) referenced |
| ODB | 0161 | error: SHIELD net ({}) does not exists. |
| ODB | 0162 | error: undefined layer ({}) referenced |
| ODB | 0163 | error: undefined ia ({}) referenced |
| ODB | 0164 | error: undefined ia ({}) referenced |
| ODB | 0165 | error: undefined layer ({}) referenced |
| ODB | 0166 | error: duplicate via ({}) |
| ODB | 0167 | error: cannot file VIA GENERATE rule in technology ({}). |
| ODB | 0168 | error: undefined layer ({}) referenced |
| ODB | 0169 | error: undefined layer ({}) referenced |
| ODB | 0170 | error: undefined layer ({}) referenced |
| ODB | 0171 | error: undefined layer ({}) referenced |
| ODB | 0172 | Cannot open DEF file ({}) for writing |
| ODB | 0173 | warning: pin {} skipped because it has no net |
| ODB | 0174 | warning: missing shield net |
| ODB | 0175 | illegal: non-orthogonal-path at Pin |
| ODB | 0176 | error: undefined layer ({}) referenced |
| ODB | 0177 | error: undefined via ({}) referenced |
| ODB | 0178 | error: undefined via ({}) referenced |
| ODB | 0179 | invalid BUSBITCHARS ({})\n |
| ODB | 0180 | duplicate LAYER ({}) ignored |
| ODB | 0181 | Skipping LAYER ({}) ; Non Routing or Cut type |
| ODB | 0182 | Skipping LAYER ({}) ; cannot understand type |
| ODB | 0183 | In layer {}, spacing layer {} not found |
| ODB | 0184 | cannot find EEQ for macro {} |
| ODB | 0185 | cannot find LEQ for macro {} |
| ODB | 0186 | macro {} references unknown site {} |
| ODB | 0187 | duplicate NON DEFAULT RULE ({}) |
| ODB | 0188 | Invalid layer name {} in NON DEFAULT RULE {} |
| ODB | 0189 | Invalid layer name {} in NONDEFAULT SPACING |
| ODB | 0190 | Invalid layer name {} in NONDEFAULT SPACING |
| ODB | 0191 | error: undefined VIA {} |
| ODB | 0192 | error: undefined VIA GENERATE RULE {} |
| ODB | 0193 | error: undefined LAYER {} |
| ODB | 0194 | Cannot add a new PIN ({}) to MACRO ({}), because the pins have already been defined. \n |
| ODB | 0195 | Invalid layer name {} in antenna info for term {} |
| ODB | 0196 | Invalid layer name {} in antenna info for term {} |
| ODB | 0197 | Invalid layer name {} in antenna info for term {} |
| ODB | 0198 | Invalid layer name {} in antenna info for term {} |
| ODB | 0199 | Invalid layer name {} in antenna info for term {} |
| ODB | 0200 | Invalid layer name {} in antenna info for term {} |
| ODB | 0201 | Invalid layer name {} in antenna info for term {} |
| ODB | 0202 | Invalid layer name {} in antenna info for term {} |
| ODB | 0203 | Invalid layer name {} in SPACING |
| ODB | 0204 | Invalid layer name {} in SPACING |
| ODB | 0205 | The LEF UNITS DATABASE MICRON convert factor ({}) is greater than the database units per micron ({}) of the current technology. |
| ODB | 0206 | error: invalid dbu-per-micron value {}; valid units (100, 200, 400, 8001000, 2000, 4000, 8000, 10000, 20000) |
| ODB | 0207 | Unknown object type for USEMINSPACING: {} |
| ODB | 0208 | VIA: duplicate VIA ({}) ignored... |
| ODB | 0209 | VIA: undefined layer ({}) in VIA ({}) |
| ODB | 0210 | error: missing VIA GENERATE rule {} |
| ODB | 0211 | error: missing LAYER {} |
| ODB | 0212 | error: missing LAYER {} |
| ODB | 0213 | error: missing LAYER {} |
| ODB | 0214 | duplicate VIARULE ({}) ignoring... |
| ODB | 0215 | error: VIARULE ({}) undefined layer {} |
| ODB | 0216 | error: undefined VIA {} in VIARULE {} |
| ODB | 0217 | duplicate VIARULE ({}) ignoring... |
| ODB | 0218 | error: VIARULE ({}) undefined layer {} |
| ODB | 0221 | {} lines parsed! |
| ODB | 0222 | Reading LEF file: {} |
| ODB | 0223 | Created {} technology layers |
| ODB | 0224 | Created {} technology vias |
| ODB | 0225 | Created {} library cells |
| ODB | 0226 | Finished LEF file:  {} |
| ODB | 0227 | Error: technology already exists |
| ODB | 0228 | Error: technology does not exists |
| ODB | 0229 | Error: library ({}) already exists |
| ODB | 0230 | Error: library ({}) already exists |
| ODB | 0231 | Error: technology already exists |
| ODB | 0232 | Error: library ({}) already exists |
| ODB | 0233 | Error: technology already exists |
| ODB | 0234 | Reading LEF file:  {} ... |
| ODB | 0235 | Error reading {} |
| ODB | 0236 | Finished LEF file:  {} |
| ODB | 0237 | Created {} technology layers |
| ODB | 0238 | Created {} technology vias |
| ODB | 0239 | Created {} library cells |
| ODB | 0240 | error: Cannot open LEF file {} |
| ODB | 0241 | error: Cannot create a via instance, via ({}) has no shapes |
| ODB | 0242 | error: Can not determine which direction to continue path, |
| ODB | 0243 | via ({}) spans above and below the current layer ({}). |
| ODB | 0244 | error: Cannot create a via instance, via ({}) has no shapes |
| ODB | 0245 | error: Net {}: Can not determine which direction to continue path, |
| ODB | 0246 | unknown incomplete layer prop of type {} |
| ODB | 0247 | skipping undefined pin {} encountered in {} DEF |
| ODB | 0248 | skipping undefined comp {} encountered in {} DEF |
| ODB | 0249 | skipping undefined net {} encountered in FLOORPLAN DEF |
| ODB | 0250 | Chip does not exist |
| ODB | 0252 | Updated {} pins. |
| ODB | 0253 | Updated {} components. |
| ODB | 0254 | Updated {} nets and {} connections. |
| ODB | 0260 | DESIGN is not defined in DEF |
| ODB | 0261 | Block with name \"{}\" already exists, renaming too \"{}\" |
| ODB | 0270 | error: Cannot open zipped LEF file {} |
| ODB | 0271 | error: Cannot open zipped DEF file {} |
| ODB | 0273 | Create ND RULE {} for layer/width {},{} |
| ODB | 0274 | Zero length path segment ({},{}) ({},{}) |
| ODB | 0275 | skipping undefined net {} encountered in FLOORPLAN DEF |
| ODB | 0276 | via ({}) spans above and below the current layer ({}). |
| ODB | 0277 | dropping LEF58_SPACING rule for cut layer {} for referencing undefined layer {} |
| ODB | 0279 | parse mismatch in layer property {} for layer {} : \"{}\" |
| ODB | 0280 | dropping LEF58_SPACINGTABLE rule for cut layer {} for referencing undefined layer {} |
| ODB | 0282 | setNumMask {} not in range [1,3] |
| ODB | 0283 | Can't open masters file {}. |
| ODB | 0284 | Master {} not found. |
| ODB | 0285 | Master {} seen more than once in {}. |
| ODB | 0286 | Terminal {} of CDL master {} not found in LEF. |
| ODB | 0287 | Master {} was not in the masters CDL files. |
| ODB | 0288 | LEF data from {} is discarded due to errors |
| ODB | 0289 | LEF data from {} is discarded due to errors |
| ODB | 0290 | LEF data discarded due to errors |
| ODB | 0291 | LEF data from {} is discarded due to errors |
| ODB | 0292 | LEF data from {} is discarded due to errors |
| ODB | 0293 | TECHNOLOGY is ignored |
| ODB | 0294 | dbInstHdrObj not expected in getDbName |
| ODB | 0295 | dbHierObj not expected in getDbName |
| ODB | 0296 | dbNameObj not expected in getDbName |
| ODB | 0297 | Physical only instance {} can't be added to module {} |
| ODB | 0298 | The top module can't be destroyed. |
| ODB | 0299 | Via.cpp:257          Via {} has only {} shapes and must have at least three. |
| ODB | 0300 | Via.cpp:267          Via {} has cut top layer {} |
| ODB | 0301 | Via.cpp:277          Via {} has cut bottom layer {} |
| ODB | 0302 | Via.cpp:293          Via {} has no cut shapes. |
| ODB | 0303 | The initial {} rows ({} sites) were cut with {} shapes for a total of {} rows ({} sites). |
| ODB | 0304 | Group.cpp:61         Group \"{}\" already exists |
| ODB | 0305 | Region \"{}\" is not found |
| ODB | 0306 | error: netlist component ({}) is not defined |
| ODB | 0307 | Guides file could not be opened. |
| ODB | 0308 | please load the design before trying to use this command |
| ODB | 0309 | duplicate group name |
| ODB | 0310 | please load the design before trying to use this command |
| ODB | 0311 | please define either top module or the modinst path |
| ODB | 0312 | module does not exist |
| ODB | 0313 | duplicate group name |
| ODB | 0314 | duplicate group name |
| ODB | 0315 | -area is a list of 4 coordinates |
| ODB | 0316 | please define area |
| ODB | 0317 | please load the design before trying to use this command |
| ODB | 0318 | duplicate region name |
| ODB | 0319 | duplicate group name |
| ODB | 0320 | please load the design before trying to use this command |
| ODB | 0321 | group does not exist |
| ODB | 0322 | group is not of physical cluster type |
| ODB | 0323 | please load the design before trying to use this command |
| ODB | 0324 | group does not exist |
| ODB | 0325 | group is not of voltage domain type |
| ODB | 0326 | define domain name |
| ODB | 0327 | define net name |
| ODB | 0328 | please load the design before trying to use this command |
| ODB | 0329 | group does not exist |
| ODB | 0330 | group is not of voltage domain type |
| ODB | 0331 | net does not exist |
| ODB | 0332 | define domain name |
| ODB | 0333 | define net name |
| ODB | 0334 | please load the design before trying to use this command |
| ODB | 0335 | group does not exist |
| ODB | 0336 | group is not of voltage domain type |
| ODB | 0337 | net does not exist |
| ODB | 0338 | please load the design before trying to use this command |
| ODB | 0339 | cluster does not exist |
| ODB | 0340 | group is not of physical cluster type |
| ODB | 0341 | modinst does not exist |
| ODB | 0342 | inst does not exist |
| ODB | 0343 | child physical cluster does not exist |
| ODB | 0344 | child group is not of physical cluster type |
| ODB | 0345 | please load the design before trying to use this command |
| ODB | 0346 | cluster does not exist |
| ODB | 0347 | group is not of physical cluster type |
| ODB | 0348 | parent module does not exist |
| ODB | 0349 | modinst does not exist |
| ODB | 0350 | inst does not exist |
| ODB | 0351 | child physical cluster does not exist |
| ODB | 0352 | child group is not of physical cluster type |
| ODB | 0353 | please load the design before trying to use this command |
| ODB | 0354 | please load the design before trying to use this command |
| ODB | 0355 | please load the design before trying to use this command |
| ODB | 0356 | dropping LEF58_METALWIDTHVIAMAP for referencing undefined layer {} |
| ODB | 0357 | Master {} was not in the masters CDL files, but master has no pins. |
| ODB | 0358 | cannot open file {} |
| ODB | 0359 | Attempt to change the origin of {} instance {} |
| ODB | 0360 | Attempt to change the orientation of {} instance {} |
| ODB | 0361 | dropping LEF58_AREA for referencing undefined layer {} |
| ODB | 0362 | Attempt to destroy dont_touch instance {} |
| ODB | 0364 | Attempt to destroy dont_touch net {} |
| ODB | 0367 | Attempt to change the module of dont_touch instance {} |
| ODB | 0368 | Attempt to change master of dont_touch instance {} |
| ODB | 0369 | Attempt to connect iterm of dont_touch instance {} |
| ODB | 0370 | Attempt to disconnect iterm of dont_touch instance {} |
| ODB | 0371 | Attempt to remove dont_touch instance {} from parent module |
| ODB | 0372 | Attempt to disconnect iterm of dont_touch net {} |
| ODB | 0373 | Attempt to connect iterm to dont_touch net {} |
| ODB | 0374 | Attempt to destroy bterm on dont_touch net {} |
| ODB | 0375 | Attempt to disconnect bterm of dont_touch net {} |
| ODB | 0376 | Attempt to create bterm on dont_touch net {} |
| ODB | 0377 | Attempt to connect bterm to dont_touch net {} |
| ODB | 0378 | Global connections are not set up. |
| ODB | 0379 | {} is marked do not touch, will be skipped for global conenctions |
| ODB | 0380 | {}/{} is connected to {} which is marked do not touch, this connection will not be modified. |
| ODB | 0381 | Invalid net specified. |
| ODB | 0382 | {} is marked do not touch, which will cause the global connect rule to be ignored. |
| ODB | 0383 | {} is marked do not touch and will be skipped in global connections. |
| ODB | 0384 | Invalid regular expression specified the {} pattern: {} |
| ODB | 0385 | Attempt to create instance with duplicate name: {} |
| ODB | 0386 | {} contains {} placed instances and will not be cut. |
| ODB | 0387 | error: Non-default rule ({}) has no rule for layer {}. |
| ODB | 0388 | unsupported {} property for layer {} :\"{}\" |
| ODB | 0389 | Cannot allocate {} MBytes for mapArray |
| ODB | 0390 | order_wires failed: net {}, shorts to another term at wire point ({} {}) |
| ODB | 0391 | order_wires failed: net {}, shorts to another term at wire point ({} {}) |
| ODB | 0392 | order_wires failed: net {}, shorts to another term at wire point ({} {}) |
| ODB | 0393 | tmg_conn::addToWire: value of k is negative: {} |
| ODB | 0394 | Cannot find instance in DB with id {} |
| ODB | 0395 | cannot order {} |
| ODB | 0396 | cannot order {} |
| ODB | 0397 | Cannot find instance in DB with name {} |
| ODB | 0398 | Cannot open file {} for writting |
| ODB | 0399 | Cannot open file {} for writting |
| ODB | 0400 | Cannot create wire, because net name is NULL\n |
| ODB | 0401 | Cannot create wire, because routing layer ({}) is invalid |
| ODB | 0402 | Cannot create net %s, because wire width ({}) is less than minWidth ({}) on layer {} |
| ODB | 0403 | Cannot create net {}, because failed to create bterms |
| ODB | 0404 | Cannot create wire, because net name is NULL |
| ODB | 0405 | Cannot create wire, because routing layer ({}) is invalid |
| ODB | 0406 | Cannot create net {}, duplicate net |
| ODB | 0407 | Cannot create net {}, because failed to create bterms |
| ODB | 0408 | Cannot create wire, because net name is NULL |
| ODB | 0409 | Cannot create net {}, duplicate net |
| ODB | 0410 | Cannot create wire, because routing layer ({}) is invalid |
| ODB | 0411 | Cannot create wire, because routing layer ({}) is invalid |
| ODB | 0412 | Cannot create wire, because routing layer ({}) is invalid |
| ODB | 0413 | Cannot create wire, because routing layer ({}) is invalid |
| ODB | 0414 | Cannot find instance term in DB with id {} |
| ODB | 0415 | Cannot find net in DB with id {} |
| ODB | 0416 | Cannot find block term in DB with id {} |
| ODB | 0417 | There is already an ECO block present! Will continue updating |
| ODB | 0418 | Cannot find net in DB with name {} |
| ODB | 0419 | Cannot find block term in DB with name {} |
| ODB | 0420 | tmg_conn::detachTilePins: tilepin inside iterm. |
| ODB | 0421 | DEF parser returns an error! |
| ODB | 0422 | DEF parser returns an error! |
| ODB | 0423 | LEF58_REGION layer {} ignored |
| ODB | 0424 | Cannot zero/negative number of chars |
| ODB | 0428 | Cannot open file {} for \"{}\" |
| ODB | 0429 | Syntax Error at line {} ({}) |
| ODB | 0430 | Layer {} has index {} which is too large to be stored |
| ODB | 0431 | Can't delete master {} which still has instances |
| ODB | 1000 | Layer ${layerName} not found, skipping NDR for this layer |
| ODB | 1001 | Layer ${firstLayer} not found |
| ODB | 1002 | Layer ${lastLayer} not found |
| ODB | 1004 | -name is missing |
| ODB | 1005 | NonDefaultRule ${name} already exists |
| ODB | 1006 | Spacing values \[$spacings\] are malformed |
| ODB | 1007 | Width values \[$widths\] are malformed |
| ODB | 1008 | Via ${viaName} not found, skipping NDR for this via |
| ODB | 1009 | Invalid input in create_ndr cmd |
| ODB | 1015 | Cannot open LEF file %s\n |
| ODB | 1033 | Cannot open LEF file %s\n |
| ODB | 1051 | Cannot open LEF file %s\n |
| ODB | 1072 | Cannot open LEF file %s\n |
| ODB | 1100 | AccessPoint.cpp:323     Access direction is of unknown type |
| ODB | 1101 | AccessPoint.cpp:343     Access direction is of unknown type |
| ODB | 1102 | Mask color: {}, but must be between 1 and 3 |
| ODB | 2000 | Cannot parse LEF property '{}' with value '{}' |
| ORD | 0001 | $filename does not exist. |
| ORD | 0002 | $filename is not readable. |
| ORD | 0003 | $filename does not exist. |
| ORD | 0004 | $filename is not readable. |
| ORD | 0005 | No technology has been read. |
| ORD | 0006 | DEF versions 5.8, 5.7, 5.6, 5.5, 5.4, 5.3 supported. |
| ORD | 0007 | $filename does not exist. |
| ORD | 0008 | $filename is not readable. |
| ORD | 0013 | Command units uninitialized. Use the read_liberty or set_cmd_units command to set units. |
| ORD | 0014 | both -setup and -hold specified. |
| ORD | 0015 | Unknown tool name {} |
| ORD | 0016 | Options -incremental, -floorplan_initialization, and -child are mutually exclusive. |
| ORD | 0017 | both -setup and -hold specified. |
| ORD | 0018 | both -setup and -hold specified. |
| ORD | 0019 | both -setup and -hold specified. |
| ORD | 0030 | Using {} thread(s). |
| ORD | 0031 | Unable to determine maximum number of threads.\nOne thread will be used. |
| ORD | 0032 | Invalid thread number specification: {}. |
| ORD | 0033 | -order_wires is deprecated. |
| ORD | 0034 | More than one lib exists, multiple files will be written. |
| ORD | 0036 | A block already exists in the db |
| ORD | 0037 | No block loaded. |
| ORD | 0038 | -gui is not yet supported with -python |
| ORD | 0039 | .openroad ignored with -python |
| ORD | 0041 | The flags -power and -ground of the add_global_connection command are mutually exclusive. |
| ORD | 0042 | The -net option of the add_global_connection command is required. |
| ORD | 0043 | The -pin_pattern option of the add_global_connection command is required. |
| ORD | 0044 | Net created for $keys(-net), if intended as power or ground net add the -power/-ground switch as appropriate. |
| ORD | 0045 | Region \"$keys(-region)\" not defined |
| ORD | 0046 | -defer_connection has been deprecated. |
| ORD | 0047 | You can't load a new db file as the db is already populated |
| ORD | 0048 | You can't load a new DEF file as the db is already populated. |
| ORD | 0101 | Only one of the options -incremental and -floorplan_init can be set at a time |
| ORD | 0101 | Use -layer or -via but not both. |
| ORD | 0102 | No technology has been read. |
| ORD | 0102 | layer $layer_name not found. |
| ORD | 0103 | $layer_name is not a routing layer. |
| ORD | 0104 | missing -capacitance or -resistance argument. |
| ORD | 0105 | Can't open {} |
| ORD | 0105 | via $layer_name not found. |
| ORD | 0106 | -capacitance not supported for vias. |
| ORD | 0108 | no -resistance specified for via. |
| ORD | 0109 | missing -layer or -via argument. |
| ORD | 1001 | LEF macro {} pin {} missing from liberty cell. |
| ORD | 1002 | Liberty cell {} pin {} missing from LEF macro. |
| ORD | 1003 | deletePin not implemented for dbITerm |
| ORD | 1004 | unimplemented network function mergeInto |
| ORD | 1005 | unimplemented network function mergeInto |
| ORD | 1006 | pin is not ITerm or BTerm |
| ORD | 1007 | unhandled port direction |
| ORD | 1008 | unknown master term type |
| ORD | 1009 | -name is missing. |
| ORD | 1009 | missing top_cell_name argument and no current_design. |
| ORD | 1010 | Either -net or -all_clocks need to be defined. |
| ORD | 1010 | no technology has been read. |
| ORD | 1011 | No NDR named ${ndrName} found. |
| ORD | 1011 | LEF master {} has no liberty cell. |
| ORD | 1012 | No net named ${netName} found. |
| ORD | 1012 | Liberty cell {} has no LEF master. |
| ORD | 1013 | -masters is required. |
| ORD | 1013 | instance {} LEF master {} not found. |
| ORD | 1014 | hierachical instance creation failed for {} of {} |
| ORD | 1015 | leaf instance creation failed for {} of {} |
| ORD | 1016 | instance is not Inst or ModInst |
| ORD | 1050 | Options -bloat and -bloat_occupied_layers are both set. At most one should be used. |
| PAD | 0001 | Unable to place {} ({}) at ({:.3f}um, {:.3f}um) - ({:.3f}um, {:.3f}um) as it overlaps with {} ({}) |
| PAD | 0002 | {}/{} ({}) and {}/{} ({}) are touching, but are connected to different nets |
| PAD | 0003 | {:.3f}um is below the minimum width for {}, changing to {:.3f}um |
| PAD | 0004 | {:.3f}um is below the minimum spacing for {}, changing to {:.3f}um |
| PAD | 0005 | Routing {} nets |
| PAD | 0006 | Failed to route the following {} nets: |
| PAD | 0007 | Failed to route {} nets. |
| PAD | 0008 | Unable to snap ({:.3f}um, {:.3f}um) to routing grid. |
| PAD | 0009 | No edges added to routing grid to access ({:.3f}um, {:.3f}um). |
| PAD | 0010 | {} only has one iterm on {} layer |
| PAD | 0011 | {} is not of type {}, but is instead {} |
| PAD | 0012 | {} is not of type {}, but is instead {} |
| PAD | 0013 | Unable to find {} row to place a corner cell in |
| PAD | 0014 | Horizontal site must be speficied. |
| PAD | 0015 | Vertical site must be speficied. |
| PAD | 0016 | Corner site must be speficied. |
| PAD | 0018 | Unable to create instance {} without master |
| PAD | 0019 | Row must be specified to place a pad |
| PAD | 0020 | Row must be specified to place IO filler |
| PAD | 0021 | Row must be specified to remove IO filler |
| PAD | 0022 | Layer must be specified to perform routing. |
| PAD | 0023 | Master must be specified. |
| PAD | 0024 | Instance must be specified to assign it to a bump. |
| PAD | 0025 | Net must be specified to assign it to a bump. |
| PAD | 0026 | Filling {} ({:.3f}um -> {:.3f}um) will result in a gap. |
| PAD | 0027 | Bond master must be specified to place bond pads |
| PAD | 0028 | Corner master must be specified. |
| PAD | 0029 | {} is not a recognized IO row. |
| PAD | 0030 | Unable to fill gap completely {:.3f}um -> {:.3f}um in row {} |
| PAD | 0031 | {} contains more than 1 pin shape on {} |
| PAD | 0032 | Unable to determine the top layer of {} |
| PAD | 0033 | Could not find a block terminal associated with net: \"{}\", creating now. |
| PAD | 0034 | Unable to create block terminal: {} |
| PAD | 0100 | Unable to find site: $name |
| PAD | 0101 | Unable to find master: $name |
| PAD | 0102 | Unable to find instance: $name |
| PAD | 0103 | Unable to find net: $name |
| PAD | 0104 | $arg is required for $cmd |
| PAD | 0105 | Unable to find layer: $keys(-layer) |
| PAD | 0106 | Unable to find row: {} |
| PAD | 0107 | Unable to find techvia: $keys(-bump_via) |
| PAD | 0108 | Unable to find techvia: $keys(-pad_via) |
| PAD | 0109 | Unable to find instance: $inst_name |
| PAD | 0110 | Unable to find iterm: $iterm_name of $inst_name |
| PAD | 0111 | Unable to find net: $net_name |
| PAD | 9001 | $str\nIncorrect signal assignments ([llength $errors]) found. |
| PAD | 9002 | Not enough bumps: available [expr 2 * ($num_signals_top_bottom + $num_signals_left_right)], required $required. |
| PAD | 9004 | Cannot find a terminal [get_padcell_signal_name $padcell] for ${padcell}. |
| PAD | 9005 | Illegal orientation \"$orient\" specified. |
| PAD | 9006 | Illegal orientation \"$orient\" specified. |
| PAD | 9007 | File $signal_assignment_file not found. |
| PAD | 9008 | Cannot find cell $name in the database. |
| PAD | 9009 | Expected 1, 2 or 4 offset values, got [llength $args]. |
| PAD | 9010 | Expected 1, 2 or 4 inner_offset values, got [llength $args]. |
| PAD | 9011 | Expected instance $name for padcell, $padcell not found. |
| PAD | 9012 | Cannot find a terminal $signal_name to associate with bondpad [$inst getName]. |
| PAD | 9014 | Net ${signal}_$section already exists, so cannot be used in the padring. |
| PAD | 9015 | No cells found on $side_name side. |
| PAD | 9016 | Scaled core area not defined. |
| PAD | 9017 | Found [llength $pad_connections] top level connections to $pin_name of padcell i$padcell (inst:[$inst getName]), expecting only 1. |
| PAD | 9018 | No terminal $signal found on $inst_name. |
| PAD | 9019 | Cannot find shape on layer [get_footprint_pad_pin_layer] for [$inst getName]:[[$inst getMaster] getName]:[$mterm getName]. |
| PAD | 9021 | Value of bump spacing_to_edge not specified. |
| PAD | 9022 | Cannot find padcell $padcell. |
| PAD | 9023 | Signal name for padcell $padcell has not been set. |
| PAD | 9024 | Cannot find bondpad type in library. |
| PAD | 9025 | No instance found for $padcell. |
| PAD | 9026 | Cannot find bondpad type in library. |
| PAD | 9027 | Illegal orientation $orientation specified. |
| PAD | 9028 | Illegal orientation $orientation specified. |
| PAD | 9029 | No types specified in the library. |
| PAD | 9030 | Unrecognized arguments to init_footprint $arglist. |
| PAD | 9031 | No die_area specified in the footprint specification. |
| PAD | 9032 | Cannot find net $signal_name for $padcell in the design. |
| PAD | 9033 | No value defined for pad_pin_name in the library or cell data for $type. |
| PAD | 9034 | No bump pitch table defined in the library. |
| PAD | 9035 | No bump_pitch defined in library data. |
| PAD | 9036 | No width defined for selected bump cell $cell_name. |
| PAD | 9037 | No bump cell defined in library data. |
| PAD | 9038 | No bump_pin_name attribute found in the library. |
| PAD | 9039 | No rdl_width defined in library data. |
| PAD | 9040 | No rdl_spacing defined in library data. |
| PAD | 9041 | AD 9041 ICeWall.tcl:685           A value for core_area must specified in the footprint specification, or in the environment variable CORE_AREA. |
| PAD | 9042 | Cannot find any pads on $side side. |
| PAD | 9043 | Pads must be defined on all sides of the die for successful extraction. |
| PAD | 9044 | Cannot open file $signal_map_file. |
| PAD | 9045 | Cannot open file $footprint_file. |
| PAD | 9046 | No power nets found in design. |
| PAD | 9047 | No ground nets found in design. |
| PAD | 9048 | No padcell instance found for $padcell. |
| PAD | 9049 | No cells defined in the library description. |
| PAD | 9050 | Multiple nets found on $signal in padring. |
| PAD | 9051 | Creating padring net: $signal_name. |
| PAD | 9052 | Creating padring net: _UNASSIGNED_$idx. |
| PAD | 9053 | Creating padring nets: [join $report_nets_created {, }]. |
| PAD | 9054 | Parameter center \"$center\" missing a value for x. |
| PAD | 9055 | Parameter center \"$center\" missing a value for y. |
| PAD | 9056 | Parameter center \"$center\" missing a value for x. |
| PAD | 9057 | Parameter center \"$center\" missing a value for y. |
| PAD | 9058 | Footprint has no padcell attribute. |
| PAD | 9059 | No side attribute specified for padcell $padcell. |
| PAD | 9060 | Cannot determine location of padcell $padcell. |
| PAD | 9061 | Footprint attribute die_area has not been defined. |
| PAD | 9062 | Footprint attribute die_area has not been defined. |
| PAD | 9063 | Padcell $padcell_name not specified. |
| PAD | 9064 | No type attribute specified for padcell $padcell_name. |
| PAD | 9065 | No type attribute specified for padcell $padcell. |
| PAD | 9066 | Library data has no type entry $type. |
| PAD | 9070 | Library does not have type $type specified. |
| PAD | 9071 | No cell $cell_name found. |
| PAD | 9072 | No bump attribute for padcell $padcell. |
| PAD | 9073 | No row attribute specified for bump associated with padcell $padcell. |
| PAD | 9074 | No col attribute specified for bump associated with padcell $padcell. |
| PAD | 9075 | No bump attribute for padcell $padcell. |
| PAD | 9076 | No row attribute specified for bump associated with padcell $padcell. |
| PAD | 9077 | No col attribute specified for bump associated with padcell $padcell. |
| PAD | 9078 | Layer [get_footprint_pin_layer] not defined in technology. |
| PAD | 9079 | Footprint does not have the pads_per_pitch attribute specified. |
| PAD | 9080 | Attribute $corner not specified in pad_ring ($pad_ring). |
| PAD | 9081 | Attribute $corner not specified in pad_ring ($pad_ring). |
| PAD | 9082 | Type $type not specified in the set of library types. |
| PAD | 9083 | Cell $cell_ref of Type $type is not specified in the list of cells in the library. |
| PAD | 9084 | Type $type not specified in the set of library types. |
| PAD | 9085 | Cell $cell_ref of Type $type is not specified in the list of cells in the library. |
| PAD | 9086 | Type $type not specified in the set of library types. |
| PAD | 9087 | Cell $cell_ref of Type $type is not specified in the list of cells in the library. |
| PAD | 9091 | No signal $signal_name defined for padcell. |
| PAD | 9092 | No signal $signal_name or $try_signal defined for padcell. |
| PAD | 9093 | Signal \"$signal_name\" not found in design. |
| PAD | 9094 | Value for -edge_name ($edge_name) not permitted, choose one of bottom, right, top or left. |
| PAD | 9095 | Value for -type ($type) does not match any library types ([dict keys [dict get $library types]]). |
| PAD | 9096 | No cell $cell_type defined in library ([dict keys [dict get $library cells]]). |
| PAD | 9097 | No entry found in library definition for cell $cell_type on $position side. |
| PAD | 9098 | No library types defined. |
| PAD | 9099 | Invalid orientation $orient, must be one of \"$valid\". |
| PAD | 9100 | Incorrect number of arguments for location, expected an even number, got [llength $location] ($location). |
| PAD | 9101 | Only one of center or origin may be specified for -location ($location). |
| PAD | 9102 | Incorrect value specified for -location center ([dict get $location center]), $msg. |
| PAD | 9103 | Incorrect value specified for -location origin ([dict get $location origin]), $msg. |
| PAD | 9104 | Required origin or center not specified for -location ($location). |
| PAD | 9105 | Specification of bondpads is only allowed for wirebond padring layouts. |
| PAD | 9106 | Specification of bumps is only allowed for flipchip padring layouts. |
| PAD | 9109 | Incorrect number of arguments for add_pad - expected an even number, received [llength $args]. |
| PAD | 9110 | Must specify -type option if -name is not specified. |
| PAD | 9111 | Unrecognized argument $arg, should be one of -pitch, -bump_pin_name, -spacing_to_edge, -cell_name, -bumps_per_tile, -rdl_layer, -rdl_width, -rdl_spacing. |
| PAD | 9112 | Padcell $padcell_duplicate already defined to use [dict get $padcell signal_name]. |
| PAD | 9113 | Type specified must be flipchip or wirebond. |
| PAD | 9114 | No origin information specified for padcell $padcell $type $inst. |
| PAD | 9115 | No origin information specified for padcell $padcell. |
| PAD | 9116 | Side for padcell $padcell cannot be determined. |
| PAD | 9117 | No orient entry for cell reference $cell_ref matching orientation $orient. |
| PAD | 9119 | No cell reference $cell_ref found in library data. |
| PAD | 9120 | Padcell $padcell does not have any location information to derive orientation. |
| PAD | 9121 | Padcell $padcell does not define orientation for $element. |
| PAD | 9122 | Cannot find an instance with name \"$inst_name\". |
| PAD | 9123 | Attribute 'name' not defined for padcell $padcell. |
| PAD | 9124 | Cell type $type does not exist in the set of library types. |
| PAD | 9125 | No type specified for padcell $padcell. |
| PAD | 9126 | Only one of center or origin should be used to specify the location of padcell $padcell. |
| PAD | 9127 | Cannot determine side for padcell $padcell, need to sepecify the location or the required edge for the padcell. |
| PAD | 9128 | No orientation specified for $cell_ref for side $side_name. |
| PAD | 9129 | Cannot determine cell name for $padcell_name from library element $cell_ref. |
| PAD | 9130 | Cell $cell_name not loaded into design. |
| PAD | 9131 | Orientation of padcell $padcell_name is $orient, which is different from the orientation expected for padcells on side $side_name ($side_from_orient). |
| PAD | 9132 | Missing orientation information for $cell_ref on side $side_name. |
| PAD | 9133 | Bondpad cell $bondpad_cell_ref not found in library definition. |
| PAD | 9134 | Unexpected value for orient attribute in library definition for $bondpad_cell_ref. |
| PAD | 9135 | Expected orientation ($expected_orient) of bondpad for padcell $padcell_name, overridden with value [dict exists $padcell bondpad orient]. |
| PAD | 9136 | Unexpected value for orient attribute in library definition for $bondpad_cell_ref. |
| PAD | 9137 | Missing orientation information for $cell_ref on side $side_name. |
| PAD | 9140 | Type [dict get $padcell type] (cell ref - $expected_cell_name) does not match specified cell_name ($cell_name) for padcell $padcell). |
| PAD | 9141 | Signal name for padcell $padcell has not been set. |
| PAD | 9142 | Unexpected number of arguments for set_die_area. |
| PAD | 9143 | Unexpected number of arguments for set_die_area. |
| PAD | 9144 | Unexpected number of arguments for set_core_area. |
| PAD | 9145 | Unexpected number of arguments for set_core_area. |
| PAD | 9146 | Layer $layer_name is not a valid layer for this technology. |
| PAD | 9147 | The pad_inst_name value must be a format string with exactly one string substitution %s. |
| PAD | 9159 | Cell reference $cell_ref not found in library, setting cell_name to $cell_ref. |
| PAD | 9160 | The pad_pin_name value must be a format string with exactly one string substitution %s. |
| PAD | 9161 | Position $position not defined for $cell_ref, expecting one of [join [dict keys [dict get $library cells $cell_ref cell_name]] {, }]. |
| PAD | 9162 | Required setting for num_pads_per_tile not found. |
| PAD | 9163 | Padcell [dict get $padcell inst_name] x location ([ord::dbu_to_microns [dict get $padcell cell scaled_center x]]) cannot connect to the bump $row,$col on the $side_name edge. The x location must satisfy [ord::dbu_to_microns $xMin] <= x <= [ord::dbu_to_microns $xMax]. |
| PAD | 9164 | Padcell [dict get $padcell inst_name] y location ([ord::dbu_to_microns [dict get $padcell cell scaled_center y]]) cannot connect to the bump $row,$col on the $side_name edge. The y location must satisfy [ord::dbu_to_microns $yMin] <= y <= [ord::dbu_to_microns $yMax]. |
| PAD | 9165 | Attribute 'name' not defined for cell $cell_inst. |
| PAD | 9166 | Type [dict get $cell_inst type] (cell ref - $expected_cell_name) does not match specified cell_name ($cell_name) for cell $name). |
| PAD | 9167 | Cell type $type does not exist in the set of library types. |
| PAD | 9168 | No type specified for cell $name. |
| PAD | 9169 | Only one of center or origin should be used to specify the location of cell $name. |
| PAD | 9170 | Cannot determine library cell name for cell $name. |
| PAD | 9171 | Cell $cell_name not loaded into design. |
| PAD | 9173 | No orientation information available for $name. |
| PAD | 9174 | Unexpected keyword in cell name specification, $msg. |
| PAD | 9175 | Unexpected keyword in orient specification, $orient_by_side. |
| PAD | 9176 | Cannot find $cell_name in the database. |
| PAD | 9177 | Pin $pin_name does not exist on cell $cell_name. |
| PAD | 9178 | Incorrect number of arguments for add_pad, expected an even number, received [llength $args]. |
| PAD | 9179 | Must specify -name option for add_libcell. |
| PAD | 9180 | Library cell reference missing name attribute. |
| PAD | 9181 | Library cell reference $cell_ref_name missing type attribute. |
| PAD | 9182 | Type of $cell_ref_name ($type) clashes with existing setting for type ([dict get $library types $type]). |
| PAD | 9183 | No specification found for which cell names to use on each side for padcell $cell_ref_name. |
| PAD | 9184 | No specification found for the orientation of cells on each side. |
| PAD | 9185 | No specification of the name of the external pin on cell_ref $cell_ref_name. |
| PAD | 9187 | Signal $break_signal not defined in the list of signals to connect by abutment. |
| PAD | 9188 | Invalid placement status $placement_status, must be one of either PLACED or FIRM. |
| PAD | 9189 | Cell $cell_name not loaded into design. |
| PAD | 9190 | -inst_name is a required argument to the place_cell command. |
| PAD | 9191 | Invalid orientation $orient specified, must be one of [join $valid_orientation {, }]. |
| PAD | 9192 | No orientation specified for $inst_name. |
| PAD | 9193 | Origin is $origin, but must be a list of 2 numbers. |
| PAD | 9194 | Invalid value specified for x value, [lindex $origin 0], $msg. |
| PAD | 9195 | Invalid value specified for y value, [lindex $origin 1], $msg. |
| PAD | 9196 | No origin specified for $inst_name. |
| PAD | 9197 | Instance $inst_name not in the design, -cell must be specified to create a new instance. |
| PAD | 9198 | Instance $inst_name expected to be $cell_name, but is actually [[$inst getMaster] getName]. |
| PAD | 9199 | Cannot create instance $inst_name of $cell_name. |
| PAD | 9200 | Unrecognized argument $arg, should be one of -name, -signal, -edge, -type, -cell, -location, -bump, -bondpad, -inst_name. |
| PAD | 9201 | Unrecognized argument $arg, should be one of -name, -type, -cell_name, -orient, -pad_pin_name, -break_signals, -physical_only. |
| PAD | 9202 | Padcell $padcell_duplicate already defined to use [dict get $padcell signal_name]. |
| PAD | 9203 | No cell type $breaker_cell_type defined. |
| PAD | 9204 | No cell [dict get $library types $breaker_cell_type] defined. |
| PAD | 9205 | Incorrect number of values specified for offsets ([llength $value]), expected 1, 2 or 4. |
| PAD | 9207 | Required type of cell ($required_type) has no libcell definition. |
| PAD | 9208 | Type option already set to [dict get $args -type], option $flag cannot be used to reset the type. |
| PAD | 9209 | The number of padcells within a pad pitch ($num_pads_per_tile) must be a number between 1 and 5. |
| PAD | 9210 | The number of padcells within a pad pitch (pitch $pitch: num_padcells: $value) must be a number between 1 and 5. |
| PAD | 9211 | No RDL layer specified. |
| PAD | 9212 | Width set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_width]), is less than the minimum width of the layer in this technology ([ord::dbu_to_microns $min_width]). |
| PAD | 9213 | Width set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_width]), is greater than the maximum width of the layer in this technology ([ord::dbu_to_microns $max_width]). |
| PAD | 9214 | Spacing set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_spacing]), is less than the required spacing for the layer in this technology ([ord::dbu_to_microns $spacing]). |
| PAD | 9215 | The number of pads within a bump pitch has not been specified. |
| PAD | 9216 | AD 9216 ICeWall.tcl:5880          A padcell with the name $padcell_name already exists. |
| PAD | 9217 | Attribute $attribute $value for padcell $name has already been used for padcell [dict get $checks $attribute $value]. |
| PAD | 9218 | Unrecognized arguments ([lindex $args 0]) specified for set_bump_options. |
| PAD | 9219 | Unrecognized arguments ([lindex $args 0]) specified for set_padring_options. |
| PAD | 9220 | Unrecognized arguments ([lindex $args 0]) specified for define_pad_cell. |
| PAD | 9221 | Unrecognized arguments ([lindex $args 0]) specified for add_pad. |
| PAD | 9222 | Unrecognized arguments ([lindex $args 0]) specified for initialize_padring. |
| PAD | 9223 | Design data must be loaded before this command. |
| PAD | 9224 | Design must be loaded before calling add_pad. |
| PAD | 9225 | Library must be loaded before calling define_pad_cell. |
| PAD | 9226 | Design must be loaded before calling set_padring_options. |
| PAD | 9227 | Design must be loaded before calling initialize_padring. |
| PAD | 9228 | Design must be loaded before calling place_cell. |
| PAD | 9229 | The value for row is $row, but must be in the range 1 - $num_bumps_y. |
| PAD | 9230 | The value for col is $col, but must be in the range 1 - $num_bumps_x. |
| PAD | 9231 | Design must be loaded before calling set_bump. |
| PAD | 9232 | The -power -ground and -net options are mutualy exclusive for the set_bump command. |
| PAD | 9233 | Required option -row missing for set_bump. |
| PAD | 9234 | Required option -col missing for set_bump. |
| PAD | 9235 | Net $net_name specified as a $type net, but has alreaqdy been defined as a [dict get $bumps nets $net_name] net. |
| PAD | 9236 | Bump $row $col is not assigned to power or ground. |
| PAD | 9237 | Unrecognized arguments ([lindex $args 0]) specified for set_bump. |
| PAD | 9238 | Trying to set bump at ($row $col) to be $net_name, but it has already been set to [dict get $bumps $row $col net]. |
| PAD | 9239 | expecting a 2 element list in the form \"number number\". |
| PAD | 9240 | Invalid coordinate specified $msg. |
| PAD | 9241 | expecting a 2 element list in the form \"number number\". |
| PAD | 9242 | Invalid array_size specified $msg. |
| PAD | 9243 | expecting a 4 element list in the form \"row <integer> col <integer>\". |
| PAD | 9244 | row value ([dict get $rowcol row]), not recognized as an integer. |
| PAD | 9245 | col value ([dict get $rowcol col]), not recognized as an integer. |
| PAD | 9246 | The use of a cover DEF is deprecated, as all RDL routes are writen to the database |
| PAD | 9247 | Cannot fit IO pads between the following anchor cells : $anchor_cell_a, $anchor_cell_b. |
| PAD | 9248 | The max_spacing constraint cannot be met for cell $padcell ($padcellRef), $max_spacing_ref needs to be adjacent to $padcellRef. |
| PAD | 9249 | The max_spacing constraint cannot be met for cell $anchor_cell_a ($padcellRef), and $anchor_cell_b ($padcellRefB), because adjacent cell displacement is larger than the constraint. |
| PAD | 9250 | No center information specified for $inst_name. |
| PAD | 9251 | Cannot find cell $name in the database. |
| PAD | 9252 | Bondpad cell [[$inst getMaster] getName], does not have the specified pin name ($pin_name) |
| PAD | 9253 | Bump cell [[$inst getMaster] getName], does not have the specified pin name ($pin_name) |
| PAD | 9254 | Unfilled gaps in the padring on $side side |
| PAD | 9255 | [ord::dbu_to_microns [lindex $gap 0]] -> [ord::dbu_to_microns [lindex $gap 1]] |
| PAD | 9256 | Padcell ring cannot be filled |
| PAD | 9257 | Cannot create bondpad instance bp_${signal_name} |
| PAD | 9258 | Routing style must be 45, 90 or under. Illegal value \"$value\" specified |
| PAD | 9259 | Via $via does not exist. |
| PAD | 9260 | No via has been defined to connect from padcells to rdl |
| PAD | 9261 | No via has been defined to connect from rdl to bump |
| PAD | 9262 | RDL path trace for $padcell (bump: $row, $col) is further from the core than the padcell pad pin |
| PAD | 9263 | RDL path has an odd number of cor-ordinates |
| PAD | 9264 | Malformed point ([lindex $points 0]) for padcell $padcell (points: $points) |
| PAD | 9265 | Malformed point ($p1) for padcell $padcell (points: $points) |
| PAD | 9266 | Malformed point ($p2) for padcell $padcell (points: $points) |
| PAD | 9267 | Malformed point ($p1) for padcell $padcell (points: $points) |
| PAD | 9268 | Malformed point ($p2) for padcell $padcell (points: $points) |
| PAD | 9269 | Malformed point ($prev) for padcell $padcell (points: $points) |
| PAD | 9270 | Malformed point ($point) for padcell $padcell (points: $points) |
| PAD | 9271 | Malformed point ([lindex $points end]) for padcell $padcell (points: $points) |
| PAR | 0001 | Hierarchical coarsening time {} seconds |
| PAR | 0002 | Number of partitions = {} |
| PAR | 0003 | UBfactor = {} |
| PAR | 0004 | Seed = {} |
| PAR | 0005 | Vertex dimensions = {} |
| PAR | 0006 | Hyperedge dimensions = {} |
| PAR | 0007 | Placement dimensions = {} |
| PAR | 0008 | Hypergraph file = {} |
| PAR | 0009 | Solution file = {} |
| PAR | 0010 | Global net threshold = {} |
| PAR | 0011 | Fixed file  = {} |
| PAR | 0012 | Community file = {} |
| PAR | 0013 | Group file = {} |
| PAR | 0014 | Placement file = {} |
| PAR | 0015 | Property 'partition_id' not found for inst {}. |
| PAR | 0016 | UBfactor = {} |
| PAR | 0017 | Seed = {} |
| PAR | 0018 | Vertex dimensions = {} |
| PAR | 0019 | Hyperedge dimensions = {} |
| PAR | 0020 | Placement dimensions = {} |
| PAR | 0021 | Timing aware flag = {} |
| PAR | 0022 | Unable to open file {}. |
| PAR | 0023 | Global net threshold = {} |
| PAR | 0024 | Top {} critical timing paths are extracted. |
| PAR | 0025 | Fence aware flag = {} |
| PAR | 0026 | fence_lx = {}, fence_ly = {}, fence_ux = {}, fence_uy = {} |
| PAR | 0027 | Fixed file  = {} |
| PAR | 0028 | Community file = {} |
| PAR | 0029 | Group file = {} |
| PAR | 0030 | Solution file = {} |
| PAR | 0031 | Number of partitions = {} |
| PAR | 0032 | UBfactor = {} |
| PAR | 0033 | Seed = {} |
| PAR | 0034 | Vertex dimensions = {} |
| PAR | 0035 | Hyperedge dimensions = {} |
| PAR | 0036 | Placement dimensions = {} |
| PAR | 0037 | Hypergraph file = {} |
| PAR | 0038 | Solution file = {} |
| PAR | 0039 | Fixed file  = {} |
| PAR | 0040 | Community file = {} |
| PAR | 0041 | Group file = {} |
| PAR | 0042 | Placement file = {} |
| PAR | 0043 | hyperedge weight factor : [ {} ] |
| PAR | 0044 | vertex weight factor : [ {} ] |
| PAR | 0045 | placement weight factor : [ {} ] |
| PAR | 0046 | net_timing_factor : {} |
| PAR | 0047 | path_timing_factor : {} |
| PAR | 0048 | path_snaking_factor : {} |
| PAR | 0049 | timing_exp_factor : {} |
| PAR | 0050 | extra_delay : {} |
| PAR | 0051 | Missing mandatory argument -read_file |
| PAR | 0052 | UBfactor = {} |
| PAR | 0053 | Seed = {} |
| PAR | 0054 | Vertex dimensions = {} |
| PAR | 0055 | Hyperedge dimensions = {} |
| PAR | 0056 | Placement dimensions = {} |
| PAR | 0057 | Guardband flag = {} |
| PAR | 0058 | Timing aware flag = {} |
| PAR | 0059 | Global net threshold = {} |
| PAR | 0060 | Top {} critical timing paths are extracted. |
| PAR | 0061 | Fence aware flag = {} |
| PAR | 0062 | fence_lx = {}, fence_ly = {}, fence_ux = {}, fence_uy = {} |
| PAR | 0063 | Fixed file  = {} |
| PAR | 0064 | Community file = {} |
| PAR | 0065 | Group file = {} |
| PAR | 0066 | Hypergraph file = {} |
| PAR | 0067 | Hypergraph_int_weight_file = {} |
| PAR | 0068 | Solution file = {} |
| PAR | 0069 | hyperedge weight factor : [ {} ] |
| PAR | 0070 | vertex weight factor : [ {} ] |
| PAR | 0071 | Unable to convert line \"{}\" to an integer in file: {} |
| PAR | 0072 | Unable to open file {}. |
| PAR | 0073 | Unable to find instance {}. |
| PAR | 0074 | Instances in partitioning ({}) does not match instances in netlist ({}). |
| PAR | 0075 | timing_exp_factor : {} |
| PAR | 0076 | extra_delay : {} |
| PAR | 0077 | hyperedge weight factor : [ {} ] |
| PAR | 0078 | vertex weight factor : [ {} ] |
| PAR | 0079 | placement weight factor : [ {} ] |
| PAR | 0080 | net_timing_factor : {} |
| PAR | 0081 | path_timing_factor : {} |
| PAR | 0082 | path_snaking_factor : {} |
| PAR | 0083 | timing_exp_factor : {} |
| PAR | 0084 | coarsen order : {} |
| PAR | 0085 | thr_coarsen_hyperedge_size_skip : {} |
| PAR | 0086 | thr_coarsen_vertices : {} |
| PAR | 0087 | thr_coarsen_hyperedges : {} |
| PAR | 0088 | coarsening_ratio : {} |
| PAR | 0089 | max_coarsen_iters : {} |
| PAR | 0090 | adj_diff_ratio : {} |
| PAR | 0091 | min_num_vertcies_each_part : {} |
| PAR | 0092 | num_initial_solutions : {} |
| PAR | 0093 | num_best_initial_solutions : {} |
| PAR | 0094 | refine_iters : {} |
| PAR | 0095 | max_moves (FM or greedy refinement) : {} |
| PAR | 0096 | early_stop_ratio : {} |
| PAR | 0097 | total_corking_passes : {} |
| PAR | 0098 | v_cycle_flag : {} |
| PAR | 0099 | max_num_vcycle : {} |
| PAR | 0100 | num_coarsen_solutions : {} |
| PAR | 0101 | num_vertices_threshold_ilp : {} |
| PAR | 0102 | Number of partitions = {} |
| PAR | 0103 | Guardband flag = {} |
| PAR | 0104 | Number of partitions = {} |
| PAR | 0105 | placement weight factor : [ {} ] |
| PAR | 0106 | net_timing_factor : {} |
| PAR | 0107 | path_timing_factor : {} |
| PAR | 0108 | path_snaking_factor : {} |
| PAR | 0109 | The runtime of multi-level partitioner : {} seconds |
| PAR | 0110 | Updated solution file name = {} |
| PAR | 0111 | This no timing-critical paths when calling GetPathTimingScore() |
| PAR | 0112 | This no timing-critical paths when calling CalculatePathsCost() |
| PAR | 0113 | This no timing-critical paths when calling GetPathsCost() |
| PAR | 0114 | This no timing-critical paths when calling GetTimingCuts() |
| PAR | 0115 | ILP-based partitioning cannot find a valid solution. |
| PAR | 0116 | ilp_accelerator_factor = {} |
| PAR | 0117 | No hyperedges will be used !!! |
| PAR | 0118 | Exit Refinement. |
| PAR | 0119 | Reset the timing_aware_flag to false. Timing-driven mode is not supported |
| PAR | 0120 | Reset the timing_aware_flag to false. Timing-driven mode is not supported |
| PAR | 0121 | no hyperedge weighting is specified. Use default value of 1. |
| PAR | 0124 | No vertex weighting is specified. Use default value of 1. |
| PAR | 0125 | No placement weighting is specified. Use default value of 1. |
| PAR | 0126 | No hyperedge weighting is specified. Use default value of 1. |
| PAR | 0127 | No vertex weighting is specified. Use default value of 1. |
| PAR | 0128 | No placement weighting is specified. Use default value of 1. |
| PAR | 0129 | Reset the fixed attributes to NONE. |
| PAR | 0130 | Reset the community attributes to NONE. |
| PAR | 0132 | Reset the placement attributes to NONE. |
| PAR | 0133 | Cannot open the fixed instance file : {} |
| PAR | 0134 | Cannot open the community file : {} |
| PAR | 0135 | Cannot open the group file : {} |
| PAR | 0136 | Timing driven partitioning is disabled |
| PAR | 0137 | {} unconstrained hyperedges ! |
| PAR | 0138 | Reset the slack of all unconstrained hyperedges to {} seconds |
| PAR | 0139 | No hyperedge weighting is specified. Use default value of 1. |
| PAR | 0140 | No placement weighting is specified. Use default value of 1. |
| PAR | 0141 | No vertex weighting is specified. Use default value of 1. |
| PAR | 0142 | This no timing-critical paths when calling GetTimingCuts() |
| PAR | 0143 | Total number of timing paths = {} |
| PAR | 0144 | Total number of timing-critical paths = {} |
| PAR | 0145 | Total number of timing-noncritical paths = {} |
| PAR | 0146 | The worst number of cuts on timing-critical paths = {} |
| PAR | 0147 | The average number of cuts on timing-critical paths = {} |
| PAR | 0148 | Total number of timing-noncritical to timing critical paths = {} |
| PAR | 0149 | The worst number of cuts on timing-non2critical paths = {} |
| PAR | 0150 | The average number of cuts on timing-non2critical paths = {} |
| PAR | 0151 | Finish Candidate Solutions Generation |
| PAR | 0152 | Finish Cut-Overlay Clustering and Optimal Partitioning |
| PAR | 0153 | Finish Vcycle Refinement |
| PAR | 0154 | [V-cycle Refinement] num_cycles = {}, cutcost = {} |
| PAR | 0155 | Number of chosen best initial solutions = {} |
| PAR | 0156 | Best initial cutcost {} |
| PAR | 0157 | Cut-Overlay Clustering : num_vertices = {}, num_hyperedges = {} |
| PAR | 0158 | Statistics of cut-overlay solution: |
| PAR | 0159 | Set ILP accelerator factor to {} |
| PAR | 0160 | Reset ILP accelerator factor to {} |
| PAR | 0161 | ilp_accelerator_factor = {} |
| PAR | 0162 | Reduce the number of hyperedges from {} to {}. |
| PAR | 0163 | Set the max_move to {} |
| PAR | 0164 | Set the refiner_iter to {} |
| PAR | 0165 | Reset the max_move to {} |
| PAR | 0166 | Reset the refiner_iters to {} |
| PAR | 0167 | Partitioning parameters****  |
| PAR | 0168 | [INFO] Partitioning parameters****  |
| PAR | 0169 | Partitioning parameters****  |
| PAR | 0170 | Partitioning parameters****  |
| PAR | 0171 | Hypergraph Information** |
| PAR | 0172 | Vertices = {} |
| PAR | 0173 | Hyperedges = {} |
| PAR | 0174 | Netlist Information** |
| PAR | 0175 | Vertices = {} |
| PAR | 0176 | Hyperedges = {} |
| PAR | 0177 | Number of timing paths = {} |
| PAR | 0178 | maximum_clock_period : {} second |
| PAR | 0179 | normalized extra delay : {} |
| PAR | 0180 | We normalized the slack of each path based on maximum clock period |
| PAR | 0181 | We normalized the slack of each net based on maximum clock period |
| PAR | 0924 | Missing mandatory argument -hypergraph_file. |
| PAR | 0925 | Missing mandatory argument -hypergraph_file. |
| PAR | 2500 | Can not open the input hypergraph file : {} |
| PAR | 2501 | Can not open the fixed file : {} |
| PAR | 2502 | Can not open the community file : {} |
| PAR | 2503 | Can not open the group file : {} |
| PAR | 2504 | Can not open the placement file : {} |
| PAR | 2511 | Can not open the solution file : {} |
| PAR | 2514 | Can not open the solution file : {} |
| PAR | 2677 | There is no vertices and hyperedges |
| PDN | 0001 | Inserting grid: {} |
| PDN | 0100 | Unable to find {} net for {} domain. |
| PDN | 0101 | Using {} as power net for {} domain. |
| PDN | 0102 | Using {} as ground net for {} domain. |
| PDN | 0103 | {} region must have a shape. |
| PDN | 0104 | {} region contains {} shapes, but only one is supported. |
| PDN | 0105 | Unable to determine location of pad offset, using die boundary instead. |
| PDN | 0106 | Width ({:.4f} um) specified for layer {} is less than minimum width ({:.4f} um). |
| PDN | 0107 | Width ({:.4f} um) specified for layer {} is greater than maximum width ({:.4f} um). |
| PDN | 0108 | Spacing ({:.4f} um) specified for layer {} is less than minimum spacing ({:.4f} um). |
| PDN | 0109 | Unable to determine width of followpin straps from standard cells. |
| PDN | 0110 | No via inserted between {} and {} at {} on {} |
| PDN | 0113 | The grid $grid_name has not been defined. |
| PDN | 0114 | Width ({:.4f} um) specified for layer {} in not a valid width, must be {}. |
| PDN | 0174 | Net $net_name has no global connections defined. |
| PDN | 0175 | Pitch {:.4f} is too small for, must be atleast {:.4f} |
| PDN | 0178 | Remaining channel {} on {} for nets: {} |
| PDN | 0179 | Unable to repair all channels. |
| PDN | 0180 | Ring cannot be build with layers following the same direction: {} |
| PDN | 0181 | Found multiple possible nets for {} net for {} domain. |
| PDN | 0182 | Instance {} already belongs to another grid \"{}\" and therefore cannot belong to \"{}\". |
| PDN | 0183 | Replacing existing core voltage domain. |
| PDN | 0184 | Cannot have region voltage domain with the same name already exists: {} |
| PDN | 0185 | Insufficient width ({} um) to add straps on layer {} in grid \"{}\ |
| "with | total | width {} um and offset {} um. |
| PDN | 0186 | Connect between layers {} and {} already exists in \"{}\". |
| PDN | 0187 | Unable to place strap on {} with unknown routing direction. |
| PDN | 0188 | Existing grid does not support adding {}. |
| PDN | 0189 | Supply pin {} of instance {} is not connected to any net. |
| PDN | 0190 | Unable to determine the pitch of the rows. |
| PDN | 0191 | {} of {:.4f} does not fit the manufacturing grid of {:.4f}. |
| PDN | 0192 | There are multiple ({}) followpin definitions in {}, but no connect statements between them. |
| PDN | 0193 | There are only ({}) followpin connect statements when {} is/are required. |
| PDN | 0194 | Connect statements for followpins overlap between layers: {} -> {} and {} -> {} |
| PDN | 0195 | Removing {} via(s) between {} and {} at ({:.4f} um, {:.4f} um) for {} |
| PDN | 0196 | {} is already defined. |
| PDN | 0197 | Unrecognized network type: {} |
| PDN | 0198 | {} requires the power cell to have an acknowledge pin. |
| PDN | 0199 | Net $switched_power_net_name already exists in the design, but is of signal type [$switched_power getSigType]. |
| PDN | 0220 | Unable to find a strap to connect power switched to. |
| PDN | 0221 | Instance {} should be {}, but is {}. |
| PDN | 0222 | Power switch insertion has already run. To reset use -ripup option. |
| PDN | 0223 | Unable to insert power switch ({}) at ({:.4f}, {:.4f}), due to lack of available rows. |
| PDN | 0224 | {} is not a net in {}. |
| PDN | 0225 | Unable to find net $net_name. |
| PDN | 0226 | {} contains block vias to be removed, which is not supported. |
| PDN | 0227 | Removing between {} and {} at ({:.4f} um, {:.4f} um) for {} |
| PDN | 0228 | Unable to open \"{}\" to write. |
| PDN | 0229 | Region must be specified. |
| PDN | 0230 | Unable to find net $net_name. |
| PDN | 1001 | The -power argument is required. |
| PDN | 1002 | Unable to find power net: $keys(-power) |
| PDN | 1003 | The -ground argument is required. |
| PDN | 1004 | Unable to find ground net: $keys(-ground) |
| PDN | 1005 | Unable to find region: $keys(-region) |
| PDN | 1006 | Unable to find secondary power net: $snet |
| PDN | 1007 | The -layer argument is required. |
| PDN | 1008 | The -width argument is required. |
| PDN | 1009 | The -pitch argument is required. |
| PDN | 1010 | Options -extend_to_core_ring and -extend_to_boundary are mutually exclusive. |
| PDN | 1011 | The -layers argument is required. |
| PDN | 1012 | Expecting a list of 2 elements for -layers option of add_pdn_ring command, found [llength $keys(-layers)]. |
| PDN | 1013 | The -widths argument is required. |
| PDN | 1014 | The -spacings argument is required. |
| PDN | 1015 | Only one of -pad_offsets or -core_offsets can be specified. |
| PDN | 1016 | One of -pad_offsets or -core_offsets must be specified. |
| PDN | 1017 | Only one of -pad_offsets or -core_offsets can be specified. |
| PDN | 1019 | The -layers argument is required. |
| PDN | 1020 | The -layers must contain two layers. |
| PDN | 1021 | Unable to find via: $via |
| PDN | 1022 | Design must be loaded before calling $args. |
| PDN | 1023 | Unable to find $name layer. |
| PDN | 1024 | $key has been deprecated$use |
| PDN | 1025 | -name is required |
| PDN | 1026 | Options -grid_over_pg_pins and -grid_over_boundary are mutually exclusive. |
| PDN | 1027 | Options -instances, -cells, and -default are mutually exclusive. |
| PDN | 1028 | Either -instances, -cells, or -default must be specified. |
| PDN | 1029 | -name is required |
| PDN | 1030 | Unable to find instance: $inst_pattern |
| PDN | 1032 | Unable to find $name domain. |
| PDN | 1033 | Argument $arg must consist of 1 or 2 entries. |
| PDN | 1034 | Argument $arg must consist of 1, 2 or 4 entries. |
| PDN | 1035 | Unknown -starts_with option: $value |
| PDN | 1036 | Invalid orientation $orient specified, must be one of [join $valid_orientations {, }]. |
| PDN | 1037 | -reset flag is mutually exclusive to all other flags |
| PDN | 1038 | -ripup flag is mutually exclusive to all other flags |
| PDN | 1039 | -report_only flag is mutually exclusive to all other flags |
| PDN | 1040 | File $config does not exist. |
| PDN | 1041 | File $config is empty. |
| PDN | 1042 | Core voltage domain will be named \"Core\". |
| PDN | 1043 | Grid named \"$keys(-name)\" already defined. |
| PDN | 1044 | Grid named \"$keys(-name)\" already defined. |
| PDN | 1045 | -power_control must be specified with -power_switch_cell |
| PDN | 1046 | Unable to find power switch cell master: $keys(-name) |
| PDN | 1047 | Unable to find $term on $master |
| PDN | 1048 | Switched power cell $keys(-power_switch_cell) is not defined. |
| PDN | 1049 | Unable to find power control net: $keys(-power_control) |
| PDN | 1183 | The -name argument is required. |
| PDN | 1184 | The -control argument is required. |
| PDN | 1186 | The -power_switchable argument is required. |
| PDN | 1187 | The -power argument is required. |
| PDN | 1188 | The -ground argument is required. |
| PDN | 1190 | Unable to find net: $keys(-net) |
| PDN | 1191 | Cannot use both -net and -all arguments. |
| PDN | 1192 | Must use either -net or -all arguments. |
| PDN | 9002 | No shapes on layer $l1 for $net. |
| PDN | 9003 | No shapes on layer $l2 for $net. |
| PDN | 9004 | Unexpected number of points in connection shape ($l1,$l2 $net [llength $points]). |
| PDN | 9006 | Unexpected number of points in stripe of $layer_name. |
| PDN | 9008 | Design name is $design_name. |
| PDN | 9009 | Reading technology data. |
| PDN | 9010 | Inserting macro grid for [llength [dict keys $instances]] macros. |
| PDN | 9011 | ****** INFO ****** |
| PDN | 9012 | **** END INFO **** |
| PDN | 9013 | Inserting stdcell grid - [dict get $specification name]. |
| PDN | 9014 | Inserting stdcell grid. |
| PDN | 9015 | Writing to database. |
| PDN | 9016 | Power Delivery Network Generator: Generating PDN\n  config: $config |
| PDN | 9017 | No stdcell grid specification found - no rails can be inserted. |
| PDN | 9018 | No macro grid specifications found - no straps added for macros. |
| PDN | 9019 | Cannot find layer $layer_name in loaded technology. |
| PDN | 9020 | Failed to read CUTCLASS property '$line'. |
| PDN | 9021 | Failed to read ENCLOSURE property '$line'. |
| PDN | 9022 | Cannot find lower metal layer $layer1. |
| PDN | 9023 | Cannot find upper metal layer $layer2. |
| PDN | 9024 | Missing logical viarule [dict get $intersection rule].\nAvailable logical viarules [dict keys $logical_viarules]. |
| PDN | 9025 | Unexpected row orientation $orient for row [$row getName]. |
| PDN | 9026 | Invalid direction \"[get_dir $layer]\" for metal layer ${layer}. Should be either \"hor\" or \"ver\". |
| PDN | 9027 | Illegal orientation $orientation specified. |
| PDN | 9028 | File $PDN_cfg is empty. |
| PDN | 9029 | Illegal number of elements defined for ::halo \"$::halo\" (1, 2 or 4 allowed). |
| PDN | 9030 | Layer specified for stdcell rails '$layer' not in list of layers. |
| PDN | 9032 | Generating blockages for TritonRoute. |
| PDN | 9033 | Unknown direction for layer $layer_name. |
| PDN | 9034 | - grid [dict get $grid_data name] for instance $instance |
| PDN | 9035 | No track information found for layer $layer_name. |
| PDN | 9036 | Attempt to add illegal via at : ([ord::dbu_to_microns [lindex $via_location 0]] [ord::dbu_to_microns [lindex $via_location 1]]), via will not be added. |
| PDN | 9037 | Pin $term_name of instance [$inst getName] is not connected to any net. |
| PDN | 9038 | Illegal via: number of cuts ($num_cuts), does not meet minimum cut rule ($min_cut_rule) for $lower_layer to $cut_class with width [ord::dbu_to_microns $lower_width]. |
| PDN | 9039 | Illegal via: number of cuts ($num_cuts), does not meet minimum cut rule ($min_cut_rule) for $upper_layer to $cut_class with width [ord::dbu_to_microns $upper_width]. |
| PDN | 9040 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full height of $layer1 ([ord::dbu_to_microns [get_grid_wire_width $layer1]]) is not covered by the overlap. |
| PDN | 9041 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full width of $layer1 ([ord::dbu_to_microns [get_grid_wire_width $layer1]]) is not covered by the overlap. |
| PDN | 9042 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full height of $layer2 ([ord::dbu_to_microns [get_grid_wire_width $layer2]]) is not covered by the overlap. |
| PDN | 9043 | No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full width of $layer2 ([ord::dbu_to_microns [get_grid_wire_width $layer2]]) is not covered by the overlap. |
| PDN | 9044 | No width information found for $layer_name. |
| PDN | 9045 | No pitch information found for $layer_name. |
| PDN | 9048 | Need to define pwr_pads and gnd_pads in config file to use pad_offset option. |
| PDN | 9051 | Infinite loop detected trying to round to grid. |
| PDN | 9052 | Unable to get channel_spacing setting for layer $layer_name. |
| PDN | 9055 | Cannot find pin $pin_name on inst [$inst getName]. |
| PDN | 9056 | Cannot find master pin $pin_name for cell [[$inst getMaster] getName]. |
| PDN | 9062 | File $PDN_cfg does not exist. |
| PDN | 9063 | Via $via_name specified in the grid specification does not exist in this technology. |
| PDN | 9064 | No power/ground pads found on bottom edge. |
| PDN | 9065 | No power/ground pads found on right edge. |
| PDN | 9066 | No power/ground pads found on top edge. |
| PDN | 9067 | No power/ground pads found on left edge. |
| PDN | 9068 | Cannot place core rings without pwr/gnd pads on each side. |
| PDN | 9069 | Cannot find via $via_name. |
| PDN | 9070 | Cannot find net $net_name in the design. |
| PDN | 9071 | Cannot create terminal for net $net_name. |
| PDN | 9072 | Design must be loaded before calling pdngen commands. |
| PDN | 9074 | Invalid orientation $orient specified, must be one of [join $valid_orientations {, }]. |
| PDN | 9075 | Layer $actual_layer_name not found in loaded technology data. |
| PDN | 9076 | Layer $layer_name not found in loaded technology data. |
| PDN | 9077 | Width ($width) specified for layer $layer_name is less than minimum width ([ord::dbu_to_microns $minWidth]). |
| PDN | 9078 | Width ($width) specified for layer $layer_name is greater than maximum width ([ord::dbu_to_microns $maxWidth]). |
| PDN | 9079 | Spacing ($spacing) specified for layer $layer_name is less than minimum spacing ([ord::dbu_to_microns $minSpacing)]. |
| PDN | 9081 | Expected an even number of elements in the list for -rails option, got [llength $rails_spec]. |
| PDN | 9083 | Expected an even number of elements in the list for straps specification, got [llength $straps_spec]. |
| PDN | 9084 | Missing width specification for strap on layer $layer_name. |
| PDN | 9085 | Pitch [dict get $straps_spec $layer_name pitch] specified for layer $layer_name is less than 2 x (width + spacing) (width=[ord::dbu_to_microns $width], spacing=[ord::dbu_to_microns $spacing]). |
| PDN | 9086 | No pitch specified for strap on layer $layer_name. |
| PDN | 9087 | Connect statement must consist of at least 2 entries. |
| PDN | 9088 | Unrecognized argument $arg, should be one of -name, -orient, -instances -cells -pins -starts_with. |
| PDN | 9090 | The orient attribute cannot be used with stdcell grids. |
| PDN | 9095 | Value specified for -starts_with option ($value), must be POWER or GROUND. |
| PDN | 9109 | Expected an even number of elements in the list for core_ring specification, got [llength $core_ring_spec]. |
| PDN | 9110 | Voltage domain $domain has not been specified, use set_voltage_domain to create this voltage domain. |
| PDN | 9111 | Instance $instance does not exist in the design. |
| PDN | 9112 | Cell $cell not loaded into the database. |
| PDN | 9114 | Unexpected value ($value), must be either POWER or GROUND. |
| PDN | 9115 | Unexpected number of values for -widths, $msg. |
| PDN | 9116 | Unexpected number of values for -spacings, $msg. |
| PDN | 9117 | Unexpected number of values for -core_offsets, $msg. |
| PDN | 9118 | Unexpected number of values for -pad_offsets, $msg. |
| PDN | 9119 | Via $via_name specified in the grid specification does not exist in this technology. |
| PDN | 9121 | Missing width specification for strap on layer $layer_name. |
| PDN | 9124 | Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect. |
| PDN | 9125 | Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect. |
| PDN | 9126 | Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect. |
| PDN | 9127 | No region $region_name found in the design for voltage_domain. |
| PDN | 9128 | Net $power_net_name already exists in the design, but is of signal type [$net getSigType]. |
| PDN | 9129 | Net $ground_net_name already exists in the design, but is of signal type [$net getSigType]. |
| PDN | 9130 | Unrecognized argument $arg, should be one of -name, -power, -ground -region. |
| PDN | 9138 | Unexpected value for direction ($direction), should be horizontal or vertical. |
| PDN | 9139 | No direction defined for layers [dict keys $core_ring_spec]. |
| PDN | 9140 | Layers [dict keys $core_ring_spec] are both $direction, missing layer in direction $other_direction. |
| PDN | 9141 | Unexpected number of directions found for layers [dict keys $core_ring_spec], ([dict keys $layer_directions]). |
| PDN | 9146 | Must specify a pad_offset or core_offset for rings. |
| PDN | 9147 | No definition of power padcells provided, required when using pad_offset. |
| PDN | 9148 | No definition of ground padcells provided, required when using pad_offset. |
| PDN | 9149 | Power net $net_name not found. |
| PDN | 9150 | Cannot find cells ([join $find_cells {, }]) in voltage domain $voltage_domain. |
| PDN | 9151 | Ground net $net_name not found. |
| PDN | 9152 | Cannot find cells ([join $find_cells {, }]) in voltage domain $voltage_domain. |
| PDN | 9153 | Core power padcell ($cell) not found in the database. |
| PDN | 9154 | Cannot find pin ($pin_name) on core power padcell ($cell). |
| PDN | 9155 | Core ground padcell ($cell) not found in the database. |
| PDN | 9156 | Cannot find pin ($pin_name) on core ground padcell ($cell). |
| PDN | 9158 | No voltage domains defined for grid. |
| PDN | 9159 | Voltage domains $domain_name has not been defined. |
| PDN | 9160 | Cannot find layer $layer_name. |
| PDN | 9164 | Problem with halo specification, $msg. |
| PDN | 9165 | Conflict found, instance $inst_name is part of two grid definitions ($grid_name, [dict get $instances $inst_name grid]). |
| PDN | 9166 | Instance $inst of cell [dict get $instance macro] is not associated with any grid. |
| PDN | 9168 | Layer $layer_name does not exist |
| PDN | 9169 | Cannot fit additional $net horizontal strap in channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin]) - ([ord::dbu_to_microns $xMax], [ord::dbu_to_microns $yMax]) |
| PDN | 9170 | Cannot fit additional $net vertical strap in channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin]) - ([ord::dbu_to_microns $xMax], [ord::dbu_to_microns $yMax]) |
| PDN | 9171 | Channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) too narrow. Channel on layer $layer_name must be at least [ord::dbu_to_microns [expr round(2.0 * $width + $channel_spacing)]] wide. |
| PDN | 9172 | Channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) too narrow. Channel on layer $layer_name must be at least [ord::dbu_to_microns [expr round(2.0 * $width + $channel_spacing)]] wide. |
| PDN | 9176 | Net $secondary_power already exists in the design, but is of signal type [$net getSigType]. |
| PDN | 9177 | Cannot find pin $term_name on instance [$inst getName] ([[$inst getMaster] getName]). |
| PDN | 9178 | Problem with max_columns specification, $msg. |
| PDN | 9179 | Problem with max_rows specification, $msg. |
| PDN | 9180 | Net $switched_power_net_name already exists in the design, but is of signal type [$net getSigType]. |
| PDN | 9181 | Switch cell $switch_cell not loaded into the database. |
| PDN | 9190 | Unrecognized argument $arg, should be one of -name, -control, -acknowledge, -power, -ground. |
| PDN | 9191 | No power control signal is defined for a grid that includes power switches |
| PDN | 9192 | Cannot find power control signal [dict get $power_switch control_signal] |
| PDN | 9193 | Cannot find instance term $control_pin for [$inst getName] of cell [[$inst getMaster] getName] |
| PDN | 9194 | Net $value does not exist in the design |
| PDN | 9195 | Option -power_control_network must be set to STAR or DAISY |
| PDN | 9196 | Invalid value specified for power control network type |
| PDN | 9197 | Cannot find pin $ack_pin_name on power switch [$inst getName] ($cell_name) |
| PDN | 9248 | Instance $instance is not associated with any grid |
| PPL | 0001 | Number of slots          {} |
| PPL | 0002 | Number of I/O            {} |
| PPL | 0003 | Number of I/O w/sink     {} |
| PPL | 0004 | Number of I/O w/o sink   {} |
| PPL | 0005 | Slots per section        {} |
| PPL | 0006 | Slots increase factor    {:.1} |
| PPL | 0007 | Random pin placement. |
| PPL | 0008 | Successfully assigned pins to sections. |
| PPL | 0009 | Unsuccessfully assigned pins to sections ({} out of {}). |
| PPL | 0010 | Tentative {} to set up sections. |
| PPL | 0012 | I/O nets HPWL: {:.2f} um. |
| PPL | 0013 | Internal error, placed more pins than exist ({} out of {}). |
| PPL | 0015 | Macro [$inst getName] is not placed. |
| PPL | 0016 | Both -direction and -pin_names constraints not allowed. |
| PPL | 0017 | -hor_layers is required. |
| PPL | 0018 | -ver_layers is required. |
| PPL | 0019 | Design without pins. |
| PPL | 0020 | Manufacturing grid is not defined. |
| PPL | 0021 | Horizontal routing tracks not found for layer $hor_layer_name. |
| PPL | 0023 | Vertical routing tracks not found for layer $ver_layer_name. |
| PPL | 0024 | Number of IO pins ({}) exceeds maximum number of available positions ({}). |
| PPL | 0025 | -exclude: $interval is an invalid region. |
| PPL | 0026 | -exclude: invalid syntax in $region. Use (top|bottom|left|right):interval. |
| PPL | 0027 | $cmd: $edge is an invalid edge. Use top, bottom, left or right. |
| PPL | 0028 | $cmd: Invalid pin direction. |
| PPL | 0029 | $cmd: Invalid edge |
| PPL | 0030 | Invalid edge for command $cmd, should be one of top, bottom, left, right. |
| PPL | 0031 | No technology found. |
| PPL | 0032 | No block found. |
| PPL | 0033 | I/O pin {} cannot be placed in the specified region. Not enough space. |
| PPL | 0034 | Pin {} has dimension {}u which is less than the min width {}u of layer {}. |
| PPL | 0036 | Number of sections is {} while the maximum recommended value is {} this may negatively affect performance. |
| PPL | 0037 | Number of slots per sections is {} while the maximum recommended value is {} this may negatively affect performance. |
| PPL | 0038 | Pin {} without net. |
| PPL | 0039 | Assigned {} pins out of {} IO pins. |
| PPL | 0040 | Negative number of slots. |
| PPL | 0041 | Pin group $group_idx: \[$group\] |
| PPL | 0042 | Unsuccessfully assigned I/O groups. |
| PPL | 0043 | Pin $pin_name not found in group $group_idx. |
| PPL | 0044 | Pin group: \[$final_group \] |
| PPL | 0045 | Layer $hor_layer_name preferred direction is not horizontal. |
| PPL | 0046 | Layer $ver_layer_name preferred direction is not vertical. |
| PPL | 0047 | Group pin $pin_name not found in the design. |
| PPL | 0048 | Restrict pins \[$names\] to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u at the $edge_name edge. |
| PPL | 0049 | Restrict $direction pins to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u, in the $edge edge. |
| PPL | 0050 | No technology has been read. |
| PPL | 0051 | Layer $layer_name not found. |
| PPL | 0052 | Routing layer not found for name $layer_name. |
| PPL | 0053 | -layer is required. |
| PPL | 0054 | -x_step and -y_step are required. |
| PPL | 0055 | -region is required. |
| PPL | 0056 | -size is not a list of 2 values. |
| PPL | 0057 | -size is required. |
| PPL | 0058 | The -pin_names argument is required when using -group flag. |
| PPL | 0059 | Box at top layer must have 4 values (llx lly urx ury). |
| PPL | 0060 | Restrict pins \[$names\] to region ([ord::dbu_to_microns $llx]u, [ord::dbu_to_microns $lly]u)-([ord::dbu_to_microns $urx]u, [ord::dbu_to_microns $urx]u) at routing layer $top_layer_name. |
| PPL | 0061 | Pins for $cmd command were not found. |
| PPL | 0063 | -region is not a list of 4 values {llx lly urx ury}. |
| PPL | 0064 | -pin_name is required. |
| PPL | 0065 | -layer is required. |
| PPL | 0066 | -location is required. |
| PPL | 0068 | -location is not a list of 2 values. |
| PPL | 0069 | -pin_size is not a list of 2 values. |
| PPL | 0070 | Pin {} placed at ({}um, {}um). |
| PPL | 0071 | Command place_pin should receive only one pin name. |
| PPL | 0072 | Number of pins ({}) exceed number of valid positions ({}). |
| PPL | 0073 | Constraint with region $region has an invalid edge. |
| PPL | 0075 | Pin {} is assigned to more than one constraint, using last defined constraint. |
| PPL | 0076 | Constraint does not have available slots for its pins. |
| PPL | 0077 | Layer {} of Pin {} not found. |
| PPL | 0078 | Not enough available positions ({}) in section ({}, {})-({}, {}) at edge {} to place the pin group of size {}. |
| PPL | 0079 | Pin {} area {:2.4f}um^2 is lesser than the minimum required area {:2.4f}um^2. |
| PPL | 0080 | Mirroring pins $pin1 and $pin2. |
| PPL | 0081 | List of pins must have an even number of pins. |
| PPL | 0082 | Mirrored position ({}, {}) at layer {} is not a valid position for pin {} placement. |
| PPL | 0083 | Both -region and -mirrored_pins constraints not allowed. |
| PPL | 0084 | Pin {} is mirrored with another pin. The constraint for this pin will be dropped. |
| PPL | 0085 | Mirrored position ({}, {}) at layer {} is not a valid position for pin placement. |
| PPL | 0087 | Both -mirrored_pins and -group constraints not allowed. |
| PPL | 0088 | Cannot assign {} constrained pins to region {}u-{}u at edge {}. Not enough space in the defined region. |
| PPL | 0089 | Could not create matrix for groups. Not available slots inside section. |
| PPL | 0090 | Group of size {} does not fit in constrained region. |
| PPL | 0091 | Mirrored position ({}, {}) at layer {} is not a valid position for pin {} placement. |
| PPL | 0092 | Pin group of size {} does not fit any section. Adding to fallback mode. |
| PPL | 0093 | Pin group of size {} does not fit in the constrained region {:.2f}-{:.2f} at {} edge. First pin of the group is {}. |
| PPL | 0094 | Cannot create group of size {}. |
| PPL | 0095 | -order cannot be used without -group. |
| PPL | 0096 | Pin group of size {} does not fit constraint region. Adding to fallback mode. |
| PPL | 0097 | The max contiguous slots ({}) is smaller than the group size ({}). |
| PPL | 0098 | Pins {} are assigned to multiple constraints. |
| PPL | 0099 | Constraint up:{$llx $lly $urx $ury} cannot be created. Pin placement grid on top layer not created. |
| PPL | 0100 | Group of size {} placed during fallback mode. |
| PPL | 0101 | Slot for position ({}, {}) in layer {} not found |
| PSM | 0001 | Reading voltage source file: {}. |
| PSM | 0002 | Output voltage file is specified as: {}. |
| PSM | 0003 | Output current file specified {}. |
| PSM | 0004 | EM calculation is enabled. |
| PSM | 0005 | Output spice file is specified as: {}. |
| PSM | 0006 | SPICE file is written at: {}. |
| PSM | 0007 | Failed to write out spice file: {}. |
| PSM | 0008 | Powergrid is not connected to all instances, therefore the IR Solver may not be accurate. LVS may also fail. |
| PSM | 0010 | LU factorization of the G Matrix failed. SparseLU solver message: {}. |
| PSM | 0012 | Solving V = inv(G)*J failed. |
| PSM | 0014 | Number of voltage sources cannot be 0. |
| PSM | 0015 | Reading location of VDD and VSS sources from {}. |
| PSM | 0016 | Voltage pad location (VSRC) file not specified, defaulting pad location to checkerboard pattern on core area. |
| PSM | 0017 | X direction bump pitch is not specified, defaulting to {}um. |
| PSM | 0018 | Y direction bump pitch is not specified, defaulting to {}um. |
| PSM | 0019 | Voltage on net {} is not explicitly set. |
| PSM | 0020 | Cannot find net {} in the design. Please provide a valid VDD/VSS net. |
| PSM | 0021 | Using voltage {:4.3f}V for ground network. |
| PSM | 0022 | Using voltage {:4.3f}V for VDD network. |
| PSM | 0024 | Instance {}, current node at ({}, {}) at layer {} have been moved from ({}, {}). |
| PSM | 0027 | Cannot find net {} in the design. Please provide a valid VDD/VSS net. |
| PSM | 0030 | VSRC location at ({:4.3f}um, {:4.3f}um) and size {:4.3f}um, is not located on an existing power stripe node. Moving to closest node at ({:4.3f}um, {:4.3f}um). |
| PSM | 0031 | Number of PDN nodes on net {} = {}. |
| PSM | 0032 | Node at ({}, {}) and layer {} moved from ({}, {}). |
| PSM | 0033 | Node at ({}, {}) and layer {} moved from ({}, {}). |
| PSM | 0035 | {} resistance not found in DB. Check the LEF or set it using the 'set_layer_rc' command. |
| PSM | 0036 | Layer {} per-unit resistance not found in DB. Check the LEF or set it using the command 'set_layer_rc -layer'. |
| PSM | 0037 | Layer {} per-unit resistance not found in DB. Check the LEF or set it using the command 'set_layer_rc -layer'. |
| PSM | 0038 | Unconnected PDN node on net {} at location ({:4.3f}um, {:4.3f}um), layer: {}. |
| PSM | 0039 | Unconnected instance {} at location ({:4.3f}um, {:4.3f}um) layer: {}. |
| PSM | 0040 | All PDN stripes on net {} are connected. |
| PSM | 0041 | Could not open SPICE file {}. Please check if it is a valid path. |
| PSM | 0042 | Unable to connect macro/pad Instance {} to the power grid. |
| PSM | 0045 | Layer {} contains no grid nodes. |
| PSM | 0046 | Node location lookup error for y. |
| PSM | 0047 | Node location lookup error for x. |
| PSM | 0048 | Printing GMat obj, with {} nodes. |
| PSM | 0049 | No nodes in object, initialization stopped. |
| PSM | 0050 | Creating stripe condunctance with invalid inputs. Min and max values for X or Y are interchanged. |
| PSM | 0051 | Index out of bound for getting G matrix conductance.  |
| PSM | 0052 | Index out of bound for getting G matrix conductance.  |
| PSM | 0053 | Cannot read $vsrc_file. |
| PSM | 0054 | Argument -net not specified. |
| PSM | 0055 | EM outfile defined without EM enable flag. Add -enable_em. |
| PSM | 0056 | No rows defined in design. Floorplan not defined. Use initialize_floorplan to add rows. |
| PSM | 0057 | Argument -net not specified. |
| PSM | 0058 | No rows defined in design. Use initialize_floorplan to add rows. |
| PSM | 0059 | Cannot read $vsrc_file. |
| PSM | 0060 | Argument -net not specified. |
| PSM | 0061 | No rows defined in design. Use initialize_floorplan to add rows and construct PDN. |
| PSM | 0062 | Argument -net or -voltage not specified. Please specify both -net and -voltage arguments. |
| PSM | 0063 | Specified bump pitches of {:4.3f} and {:4.3f} are less than core width of {:4.3f} or core height of {:4.3f}. Changing bump location to the center of the die at ({:4.3f}, {:4.3f}). |
| PSM | 0064 | Number of voltage sources = {}. |
| PSM | 0065 | VSRC location not specified, using default checkerboard pattern with one VDD every size bumps in x-direction and one in two bumps in the y-direction |
| PSM | 0066 | Layer {} per-unit resistance not found in DB. Check the LEF or set it using the command 'set_layer_rc -layer'. |
| PSM | 0067 | Multiple voltage supply values mappedat the same node ({:4.3f}um, {:4.3f}um).If you provided a vsrc file. Check for duplicate entries.Choosing voltage value {:4.3f}. |
| PSM | 0068 | Minimum resolution not set. Please run analyze_power_grid first. |
| PSM | 0069 | Check connectivity failed. |
| PSM | 0070 | Net {} has no nodes and will be skipped |
| PSM | 0071 | Instance {} is not placed. Therefore, the power drawn by this instance is not considered for IR  drop estimation. Please run analyze_power_grid after instances are placed. |
| PSM | 0072 | No nodes found in macro/pad bounding box for Instance {}.Using nearest node at ({}, {}) on the pin layer at routing level {}. |
| PSM | 0073 | Setting lower metal node density to {}um as specfied by user. |
| PSM | 0074 | No nodes found in macro or pad bounding box for Instance {} for the pin layer at routing level {}. Using layer {}. |
| PSM | 0075 | Expected four values on line: {} |
| PSM | 0076 | Setting metal node density to be standard cell height times {}. |
| PSM | 0077 | Cannot use both node_density and node_density_factor together. Use any one argument |
| PSM | 0078 | IR drop setup failed.  Analysis can't proceed. |
| PSM | 0079 | Can't determine the supply voltage as no Liberty is loaded. |
| PSM | 0080 | Creating stripe condunctance with invalid inputs. Min and max values for X or Y are interchanged. |
| PSM | 0081 | Via connection failed at {}, {} |
| PSM | 0082 | Unable to find a row |
| PSM | 0083 | Error file is specified as: {}. |
| RCX | 0001 | Reading SPEF file: {} |
| RCX | 0002 | Filename is not defined! |
| RCX | 0003 | Read SPEF into extracted db! |
| RCX | 0004 | There is no extraction db! |
| RCX | 0005 | Can't open SPEF file {} to write. |
| RCX | 0007 | Finished {} measurements for pattern MET_UNDER_MET |
| RCX | 0008 | extracting parasitics of {} ... |
| RCX | 0015 | Finished extracting {}. |
| RCX | 0016 | Writing SPEF ... |
| RCX | 0017 | Finished writing SPEF ... |
| RCX | 0019 | diffing spef {} |
| RCX | 0021 | calibrate on spef file  {} |
| RCX | 0029 | Defined extraction corner {} |
| RCX | 0030 | The original process corner name is required |
| RCX | 0031 | Defined Derived extraction corner {} |
| RCX | 0040 | Final {} rc segments |
| RCX | 0042 | {} nets finished |
| RCX | 0043 | {} wires to be extracted |
| RCX | 0044 | {} spef insts not found in db. |
| RCX | 0045 | Extract {} nets, {} rsegs, {} caps, {} ccs |
| RCX | 0047 | {} nets finished |
| RCX | 0048 | {} db nets not read from spef. |
| RCX | 0049 | {} db insts not read from spef. |
| RCX | 0050 | {} spef nets not found in db. |
| RCX | 0052 | Unmatched spef and db! |
| RCX | 0055 | Finished {} bench measurements for pattern MET_OVER_MET |
| RCX | 0057 | Finished {} bench measurements for pattern MET_UNDER_MET |
| RCX | 0058 | Finished {} bench measurements for pattern MET_DIAGUNDER_MET |
| RCX | 0060 | merged {} coupling caps |
| RCX | 0065 | layer {} |
| RCX | 0069 | {} |
| RCX | 0072 | Can't find <OVER> rules for {} |
| RCX | 0074 | Cannot find net {} from the {} table entry {} |
| RCX | 0076 | Cap Node {} not extracted |
| RCX | 0077 | SpefIn.cpp:764         Spef net {} not found in db. |
| RCX | 0078 | Break simple loop of {} nets |
| RCX | 0079 | Have processed {} CC caps, and stored {} CC caps |
| RCX | 0081 | Die Area for the block has 0 size, or is undefined! |
| RCX | 0082 | Layer {}, routing level {}, has pitch {}!! |
| RCX | 0107 | Nothing is extracted out of {} nets! |
| RCX | 0108 | Net {} multiple-ended at bterm {} |
| RCX | 0109 | Net {} multiple-ended at iterm {} |
| RCX | 0110 | Net {} has no wires. |
| RCX | 0111 | Net {} {} has a loop at x={} y={} {}. |
| RCX | 0112 | Can't locate bterm {} |
| RCX | 0113 | Can't locate iterm {}/{} ( {} ) |
| RCX | 0114 | Net {} {} does not start from an iterm or a bterm. |
| RCX | 0115 | Net {} {} already has rseg! |
| RCX | 0120 | No matching process corner for scaled corner {}, model {} |
| RCX | 0121 | The corresponding process corner has to be defined using the command <define_process_corner> |
| RCX | 0122 | A process corner for Extraction RC Model {} has already been defined, skipping definition |
| RCX | 0127 | No RC model was read with command <load_model>, will not perform extraction! |
| RCX | 0128 | skipping Extraction ... |
| RCX | 0129 | Wrong combination of corner related options! |
| RCX | 0134 | Can't execute write_spef command. There's no extraction data. |
| RCX | 0135 | Corner {} is out of range; There are {} corners in DB! |
| RCX | 0136 | Can't find corner name {} in the parasitics DB! |
| RCX | 0137 | Can't open file \"{}\" to write spef. |
| RCX | 0138 | {} layers are missing resistance value; Check LEF file. Extraction cannot proceed! Exiting |
| RCX | 0139 | Missing Resistance value for layer {} |
| RCX | 0140 | Have processed {} total segments, {} signal segments, {} CC caps, and stored {} CC caps |
| RCX | 0141 | Context of layer {} xy={} len={} base={} width={} |
| RCX | 0142 | layer {} |
| RCX | 0143 | {}: {} |
| RCX | 0147 | bench_verilog: file is not defined! |
| RCX | 0148 | Extraction corner {} not found! |
| RCX | 0149 | -db is deprecated. |
| RCX | 0152 | Can't determine Top Width for Conductor <{}> |
| RCX | 0153 | Can't determine thickness for Conductor <{}> |
| RCX | 0154 | Can't determine thickness for Diel <{}> |
| RCX | 0158 | Can't determine Bottom Width for Conductor <{}> |
| RCX | 0159 | Can't open file {} with permissions <{}> |
| RCX | 0171 | Can't open log file diff_spef.log for writing! |
| RCX | 0172 | Can't open output file diff_spef.out for writing! |
| RCX | 0175 | Non-symmetric case feature is not implemented! |
| RCX | 0176 | Skip instance {} for cell {} is excluded |
| RCX | 0178 | \" -N {} \" is unknown. |
| RCX | 0208 | {} {} {} {}  {} |
| RCX | 0216 | Can't find <UNDER> rules for {} |
| RCX | 0217 | Can't find <OVERUNDER> rules for {} |
| RCX | 0218 | Cannot write <OVER> rules for <DensityModel> {} and layer {} |
| RCX | 0219 | Cannot write <UNDER> rules for <DensityModel> {} and layer {} |
| RCX | 0220 | Cannot write <DIAGUNDER> rules for <DensityModel> {} and layer {} |
| RCX | 0221 | Cannot write <OVERUNDER> rules for <DensityModel> {} and layer {} |
| RCX | 0222 | There were {} extraction models defined but only {} exists in the extraction rules file {} |
| RCX | 0223 | Cannot find model index {} in extRules file {} |
| RCX | 0239 | Zero rseg wire property {} on main net {} {} |
| RCX | 0240 | GndCap: cannot find rseg for rsegId {} on net {} {} |
| RCX | 0252 | Ext object on dbBlock is NULL! |
| RCX | 0258 | SpefIn.cpp:137         Spef instance {} not found in db. |
| RCX | 0259 | Can't find bterm {} in db. |
| RCX | 0260 | {} and {} are connected to a coupling cap of net {} {} in spef, but connected to net {} {} and net {} {} respectively in db. |
| RCX | 0261 | Cap Node {} not extracted |
| RCX | 0262 | Iterm {}/{} is connected to net {} {} in spef, but connected to net {} {} in db. |
| RCX | 0263 | Bterm {} is connected to net {} {} in spef, but connected to net {} {} in db. |
| RCX | 0264 | SpefIn.cpp:901         Spef net {} not found in db. |
| RCX | 0265 | There is cc cap between net {} and net {} in db, but not in reference spef file |
| RCX | 0266 | There is cc cap between net {} and net {} in reference spef file, but not in db |
| RCX | 0267 | {} has no shapes! |
| RCX | 0269 | Cannot find coords of driver capNode {} of net {} {} |
| RCX | 0270 | Driving node of net {} {} is not connected to a rseg. |
| RCX | 0271 | Cannot find node coords for targetCapNodeId {} of net {} {} |
| RCX | 0272 | RCX 0272 extSpefIn.cpp:1629        RC of net {} {} is disconnected! |
| RCX | 0273 | Failed to identify loop in net {} {} |
| RCX | 0274 | {} capNodes loop in net {} {} |
| RCX | 0275 | id={} |
| RCX | 0276 | cap-{}={} |
| RCX | 0277 | Break one simple loop of {}-rsegs net {} {} |
| RCX | 0278 | {}-rsegs net {} {} has a {}-rsegs loop |
| RCX | 0279 | {}-rsegs net {} {} has {} loops |
| RCX | 0280 | Net {} {} has rseg before reading spef |
| RCX | 0281 | \"-N s\" in read_spef command, but no coordinates in spef file. |
| RCX | 0282 | Source capnode {} is the same as target capnode {}. Add the cc capacitance to ground. |
| RCX | 0283 | Have read {} nets |
| RCX | 0284 | There are {} nets with looped spef rc |
| RCX | 0285 | Break simple loop of {} nets |
| RCX | 0286 | Number of corners in SPEF file = 0. |
| RCX | 0287 | Cannot find corner name {} in DB |
| RCX | 0288 | Ext corner {} out of range; There are only {} defined process corners. |
| RCX | 0289 | Mismatch on the numbers of corners: Spef file has {} corners vs. Process corner table has {} corners.(Use -spef_corner option). |
| RCX | 0290 | SpefIn.cpp:2386        Spef corner {} out of range; There are only {} corners in Spef file |
| RCX | 0291 | Have to specify option _db_corner_name |
| RCX | 0292 | {} nets with looped spef rc |
| RCX | 0293 | *{}{}{} |
| RCX | 0294 | First {} cc that appear {} times |
| RCX | 0295 | There is no *PORTS section |
| RCX | 0296 | There is no *NAME_MAP section |
| RCX | 0297 | There is no *NAME_MAP section |
| RCX | 0298 | There is no *PORTS section |
| RCX | 0357 | No LEF technology has been read. |
| RCX | 0358 | Can't find <RESOVER> Res rules for {} |
| RCX | 0374 | Inconsistency in RC of net {} {}. |
| RCX | 0376 | DB created {} nets, {} rsegs, {} caps, {} ccs |
| RCX | 0378 | Can't open file {} |
| RCX | 0380 | Filename is not defined to run diff_spef command! |
| RCX | 0381 | Filename for calibration is not defined. Define the filename using -spef_file |
| RCX | 0404 | Cannot find corner name {} in DB |
| RCX | 0405 | {}-rsegs net {} {} has {} loops |
| RCX | 0406 | Break one simple loop of {}-rsegs net {} {} |
| RCX | 0407 | {}-rsegs net {} {} has a {}-rsegs loop |
| RCX | 0410 | Cannot write <OVER> Res rules for <DensityModel> {} and layer {} |
| RCX | 0411 | \ttotFrCap for netId {}({}) {} |
| RCX | 0414 | \tfrCap from CC for netId {}({}) {} |
| RCX | 0416 | \tccCap for netIds {}({}), {}({}) {} |
| RCX | 0417 | FrCap for netId {} (nodeId= {})  {} |
| RCX | 0418 | Reads only {} nodes from {} |
| RCX | 0431 | Defined process_corner {} with ext_model_index {} |
| RCX | 0433 | A process corner {} for Extraction RC Model {} has already been defined, skipping definition |
| RCX | 0434 | Defined process_corner {} with ext_model_index {} (using extRulesFile defaults) |
| RCX | 0435 | Reading extraction model file {} ... |
| RCX | 0436 | RCX 0436 netRC.cpp:1807            RC segment generation {} (max_merge_res {:.1f}) ... |
| RCX | 0437 | RECT {} ( {} {} ) ( {} {} )  jids= ( {} {} ) |
| RCX | 0438 | VIA {} ( {} {} )  jids= ( {} {} ) |
| RCX | 0439 | Coupling Cap extraction {} ... |
| RCX | 0440 | Coupling threshhold is {:.4f} fF, coupling capacitance less than {:.4f} fF will be grounded. |
| RCX | 0442 | {:d}% completion -- {:d} wires have been extracted |
| RCX | 0443 | {} nets finished |
| RCX | 0444 | Read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps |
| RCX | 0445 | Have read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps |
| RCX | 0447 | Db inst {} {} not read from spef file! |
| RCX | 0448 | Db net {} {} not read from spef file! |
| RCX | 0449 | {}  |
| RCX | 0451 | {} |
| RCX | 0452 | SpefIn.cpp:69          Spef instance {} not found in db. |
| RCX | 0456 | pixelTable gave len {}, bigger than expected {} |
| RCX | 0458 | pixelTable gave len {}, bigger than expected {} |
| RCX | 0459 | getOverUnderIndex: out of range n= {}   m={} u= {} o= {} |
| RCX | 0460 | Cannot find dbRseg for net {} from the {} table entry {} |
| RCX | 0463 | Have read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps |
| RCX | 0464 | Broke {} coupling caps of {} fF or smaller |
| RCX | 0465 | {} nets finished |
| RCX | 0468 | Can't open extraction model file {} |
| RCX | 0472 | The corresponding process corner has to be defined using the command <define_process_corner> |
| RCX | 0474 | Can't execute write_spef command. There's no block in db! |
| RCX | 0475 | Can't execute write_spef command. There's no block in db |
| RCX | 0476 | {}: {} |
| RCX | 0480 | cc appearance count -- 1:{} 2:{} 3:{} 4:{} 5:{} 6:{} 7:{} 8:{} 9:{} 10:{} 11:{} 12:{} 13:{} 14:{} 15:{} 16:{} |
| RCX | 0485 | Cannot open file {} with permissions {} |
| RCX | 0487 | No RC model read from the extraction model! Ensure the right extRules file is used! |
| RCX | 0489 | mv failed: {} |
| RCX | 0490 | system failed: {} |
| RCX | 0491 | rm failed on {} |
| RCX | 0497 | No design is loaded. |
| RMP | 0001 | Cannot open file {}. |
| RMP | 0002 | Blif writer successfully dumped file with {} instances. |
| RMP | 0003 | Cannot open file {}. |
| RMP | 0004 | Cannot open file {}. |
| RMP | 0005 | Blif parsed successfully, will destroy {} existing instances. |
| RMP | 0006 | Found {} inputs, {} outputs, {} clocks, {} combinational gates, {} registers after parsing the blif file. |
| RMP | 0007 | Inserting {} new instances. |
| RMP | 0008 | Const driver {} doesn't have any connected nets. |
| RMP | 0009 | Master ({}) not found while stitching back instances. |
| RMP | 0010 | Connection {} parsing failed for {} instance. |
| RMP | 0012 | Missing argument -liberty_file |
| RMP | 0016 | cannot open file {} |
| RMP | 0020 | Cannot open file {} for writing. |
| RMP | 0021 | All re-synthesis runs discarded, keeping original netlist. |
| RMP | 0025 | ABC run failed, see log file {} for details. |
| RMP | 0026 | Error executing ABC command {}. |
| RMP | 0032 | -tielo_port not specified |
| RMP | 0033 | -tiehi_port not specified |
| RMP | 0034 | Blif parser failed. File doesn't follow blif spec. |
| RMP | 0035 | Could not create instance {}. |
| RMP | 0036 | Mode {} not recognized. |
| RMP | 0076 | Could not create new instance of type {} with name {}. |
| RMP | 0146 | Could not connect instance of cell type {} to {} net due to unknown mterm in blif. |
| RSZ | 0001 | Use -layer or -resistance/-capacitance but not both. |
| RSZ | 0002 | layer $layer_name not found. |
| RSZ | 0003 | missing -placement or -global_routing flag. |
| RSZ | 0004 | -max_utilization must be between 0 and 100%. |
| RSZ | 0005 | Run global_route before estimating parasitics for global routing. |
| RSZ | 0010 | $signal_clk wire resistance is 0. |
| RSZ | 0011 | $signal_clk wire capacitance is 0. |
| RSZ | 0014 | wire capacitance for corner [$corner name] is zero. Use the set_wire_rc command to set wire resistance and capacitance. |
| RSZ | 0020 | found $floating_net_count floating nets. |
| RSZ | 0021 | no estimated parasitics. Using wire load models. |
| RSZ | 0022 | no buffers found. |
| RSZ | 0025 | max utilization reached. |
| RSZ | 0026 | Removed {} buffers. |
| RSZ | 0027 | Inserted {} input buffers. |
| RSZ | 0028 | Inserted {} output buffers. |
| RSZ | 0030 | Inserted {} buffers. |
| RSZ | 0031 | Resized {} instances. |
| RSZ | 0032 | Inserted {} hold buffers. |
| RSZ | 0033 | No hold violations found. |
| RSZ | 0034 | Found {} slew violations. |
| RSZ | 0035 | Found {} fanout violations. |
| RSZ | 0036 | Found {} capacitance violations. |
| RSZ | 0037 | Found {} long wires. |
| RSZ | 0038 | Inserted {} buffers in {} nets. |
| RSZ | 0039 | Resized {} instances. |
| RSZ | 0040 | Inserted {} buffers. |
| RSZ | 0041 | Resized {} instances. |
| RSZ | 0042 | Inserted {} tie {} instances. |
| RSZ | 0043 | Swapped pins on {} instances. |
| RSZ | 0044 | Swapped pins on {} instances. |
| RSZ | 0046 | Found {} endpoints with hold violations. |
| RSZ | 0047 | Found {} long wires. |
| RSZ | 0048 | Inserted {} buffers in {} nets. |
| RSZ | 0050 | Max utilization reached. |
| RSZ | 0051 | Found {} slew violations. |
| RSZ | 0052 | Found {} fanout violations. |
| RSZ | 0053 | Found {} capacitance violations. |
| RSZ | 0054 | Found {} long wires. |
| RSZ | 0055 | Inserted {} buffers in {} nets. |
| RSZ | 0056 | Resized {} instances. |
| RSZ | 0057 | Resized {} instances. |
| RSZ | 0058 | Using max wire length [format %.0f [sta::distance_sta_ui $max_wire_length]]um. |
| RSZ | 0060 | Max buffer count reached. |
| RSZ | 0061 | $signal_clk wire resistance [sta::format_resistance [expr $wire_res * 1e-6] 6] [sta::unit_scale_abreviation resistance][sta::unit_suffix resistance]/um capacitance [sta::format_capacitance [expr $wire_cap * 1e-6] 6] [sta::unit_scale_abreviation capacitance][sta::unit_suffix capacitance]/um. |
| RSZ | 0062 | Unable to repair all setup violations. |
| RSZ | 0064 | Unable to repair all hold checks within margin. |
| RSZ | 0065 | max wire length less than [format %.0fu [sta::distance_sta_ui $min_delay_max_wire_length]] increases wire delays. |
| RSZ | 0066 | Unable to repair all hold violations. |
| RSZ | 0067 | $key must be  between 0 and 100 percent. |
| RSZ | 0068 | missing target load cap. |
| RSZ | 0069 | skipping net {} with {} pins. |
| RSZ | 0070 | no LEF cell for {}. |
| RSZ | 0071 | unhandled BufferedNet type |
| RSZ | 0072 | unhandled BufferedNet type |
| RSZ | 0073 | driver pin {} not found in global routes |
| RSZ | 0074 | driver pin {} not found in global route grid points |
| RSZ | 0075 | makeBufferedNet failed for driver {} |
| RSZ | 0076 | -slack_margin is deprecated. Use -setup_margin/-hold_margin |
| RSZ | 0077 | some buffers were moved inside the core. |
| RSZ | 0078 | incorrect BufferedNet type {} |
| RSZ | 0079 | incorrect BufferedNet type {} |
| RSZ | 0080 | incorrect BufferedNet type {} |
| RSZ | 0081 | incorrect BufferedNet type {} |
| RSZ | 0082 | wireRC called for non-wire |
| RSZ | 0083 | pin outside regions |
| RSZ | 0084 | Output {} can't be buffered due to dont-touch driver {} |
| RSZ | 0085 | Input {} can't be buffered due to dont-touch fanout {} |
| RSZ | 0086 | metersToDbu({}) cannot convert negative distances |
| RSZ | 0088 | Corner: {} has no wire signal resistance value. |
| RSZ | 0089 | Could not find a resistance value for any corner. Cannot evaluate max wire length for buffer. Check over your `set_wire_rc` configuration |
| RSZ | 0090 | Opendp was not initialized before inserting a new instance |
| RSZ | 0091 | Opendp was not initialized before resized an instance |
| STA | 1000 | instance {} swap master {} is not equivalent |
| STT | 0001 | The alpha value must be between 0.0 and 1.0. |
| STT | 0002 | set_routing_alpha: Wrong number of arguments. |
| STT | 0003 | Nets for $cmd command were not found |
| STT | 0004 | Net {} is connected to unplaced instance {}. |
| STT | 0005 | Net {} is connected to unplaced pin {}. |
| STT | 0006 | Clock nets for $cmd command were not found |
| TAP | 0004 | Inserted {} endcaps. |
| TAP | 0005 | Inserted {} tapcells. |
| TAP | 0006 | Inserted {} top/bottom cells. |
| TAP | 0007 | Inserted {} cells near blockages. |
| TAP | 0009 | Row {} has enough space for only one endcap. |
| TAP | 0010 | Master $tapcell_master_name not found. |
| TAP | 0011 | Master $endcap_master_name not found. |
| TAP | 0012 | Master {} not found. |
| TAP | 0013 | Master {} not found. |
| TAP | 0014 | endcap_cpp option is deprecated. |
| TAP | 0015 | tbtie_cpp option is deprecated. |
| TAP | 0016 | no_cell_at_top_bottom option is deprecated. |
| TAP | 0018 | Master {} not found. |
| TAP | 0019 | Master {} not found. |
| TAP | 0020 | Master {} not found. |
| TAP | 0021 | Master $tap_nwouttie_master_name not found. |
| TAP | 0022 | Master {} not found. |
| TAP | 0023 | Master {} not found. |
| TAP | 0024 | Master {} not found. |
| TAP | 0025 | Master {} not found. |
| TAP | 0026 | Master {} not found. |
| TAP | 0027 | Master {} not found. |
| TAP | 0028 | Master {} not found. |
| TAP | 0029 | Master {} not found. |
| TAP | 0030 | Master {} not found. |
| TAP | 0031 | Master {} not found. |
| TAP | 0032 | Macro {} is not placed. |
| TAP | 0033 | Not able to build instance {} with master {}. |
| TAP | 0034 | Master $endcap_master_name not found. |
| TAP | 0100 | Removed $taps_removed tapcells. |
| TAP | 0101 | Removed $endcaps_removed endcaps. |
| UPF | 0001 | -area is a list of 4 coordinates |
| UPF | 0002 | please define area |
| UPF | 10001 | Creation of '%s' power domain failed |
| UPF | 10002 | Couldn't retrieve power domain '%s' while adding element '%s' |
| UPF | 10003 | Creation of '%s' logic port failed |
| UPF | 10004 | Couldn't retrieve power domain '%s' while creating power switch '%s' |
| UPF | 10005 | Creation of '%s' power switch failed |
| UPF | 10006 | Couldn't retrieve power switch '%s' while adding control port '%s' |
| UPF | 10007 | Couldn't retrieve power switch '%s' while adding on state '%s' |
| UPF | 10008 | Couldn't retrieve power domain '%s' while creating/updating isolation '%s' |
| UPF | 10009 | Couldn't update a non existing isolation %s |
| UPF | 10010 | Couldn't retrieve power domain '%s' while updating isolation '%s' |
| UPF | 10011 | Couldn't find isolation %s |
| UPF | 10012 | Couldn't retrieve power domain '%s' while updating its area  |
| UPF | 10013 | isolation cell has no enable port |
| UPF | 10014 | unknown isolation cell function |
| UPF | 10015 | multiple power domain definitions for the same path %s |
| UPF | 10016 | Creation of '%s' region failed |
| UPF | 10017 | No area specified for '%s' power domain |
| UPF | 10018 | Creation of '%s' group failed, duplicate group exists. |
| UPF | 10019 | Creation of '{}' dbNet from UPF Logic Port failed |
| UPF | 10020 | Isolation %s defined, but no cells defined. |
| UPF | 10021 | Isolation %s cells defined, but can't find any in the lib. |
| UPF | 10022 | Isolation %s cells defined, but can't find one of output, data or enable terms. |
| UPF | 10023 | Isolation {} has nonexisting control net {} |
| UPF | 10024 | Isolation %s has location %s, but only self|parent|fanoutsupported, defaulting to self. |
| UPF | 10025 | No TOP DOMAIN found, aborting |
| UPF | 10026 | Multiple isolation strategies defined for the same power domain %s. |
| UPF | 10027 | can't find any inverters |
| UTL | 0001 | seeking file to start {} |
| UTL | 0002 | error reading {} bytes from file at offset {}: {} |
| UTL | 0003 | read no bytes from file at offset {}, but neither error nor EOF |
| UTL | 0004 | error writing {} bytes from file at offset {}: {} |
| UTL | 0005 | could not create temp file |
| UTL | 0006 | ScopedTemporaryFile; fd: {} path: {} |
| UTL | 0007 | could not open temp descriptor as FILE |
| UTL | 0008 | could not unlink temp file at {} |
| UTL | 0009 | could not close temp file: {} |
