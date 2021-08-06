set env RESULTS_OAS 
set env RUN_ME_SCRIPT run-me-bp_fe-nangate45-base.sh
set env PLACE_DENSITY_MAX_POST_HOLD 0.12
set env KLAYOUT_LVS_FILE ./platforms/nangate45/lvs/FreePDK45.lylvs
set env WRAPPED_LEFS 
set env OPENROAD_CMD /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/tools/install/OpenROAD/bin/openroad -exit -no_init 
set env CURDIR /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/flow
set env SHELL /bin/bash
set env OPENROAD_EXE /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/tools/install/OpenROAD/bin/openroad
set env MAX_ROUTING_LAYER metal10
set env FLOW_HOME .
set env DESIGN_CONFIG ./designs/nangate45/bp_fe_top/config.mk
set env WRAPPED_LIBS 
set env MAKEFILE_LIST  Makefile designs/nangate45/bp_fe_top/config.mk platforms/nangate45/config.mk util/utils.mk
set env RESULTS_GDS 
set env WIRE_RC_LAYER metal3
set env CORE_AREA 10.07 9.8 989.9 789.6
set env MACRO_PLACE_CHANNEL 18.8 19.95
set env TAPCELL_TCL ./platforms/nangate45/tapcell.tcl
set env LATCH_MAP_FILE ./platforms/nangate45/cells_latch.v
set env ISSUE_SCRIPTS add_routing_blk cdl cts deleteNonClkNets deletePowerNets deleteRoutingObstructions density_fill detail_place detail_route fillcell final_report floorplan generate_abstract generate_lef global_place global_route gui io_placement io_placement_random klayout macro_place pdn placement_blockages read_macro_placement report_metrics resize run_all synth synth_hier_report tapcell tdms_place view_cells write_ref_sdc yosys
set env 6_FINAL_FILE ./results/nangate45/bp_fe/base/6_final.gds
set env ISSUE_TAG bp_fe_nangate45_base_2021-08-05_10-39
set env DONT_USE_SC_LIB ./objects/nangate45/bp_fe/base/lib/NangateOpenCellLibrary_typical.lib
set env OBJECTS_DIR ./objects/nangate45/bp_fe/base
set env MIN_ROUTING_LAYER metal2
set env SDC_FILE ./designs/nangate45/bp_fe_top/constraint.sdc
set env IO_PIN_MARGIN 70
set env RCX_RULES ./platforms/nangate45/rcx_patterns.rules
set env LIB_FILES ./platforms/nangate45/lib/NangateOpenCellLibrary_typical.lib ./designs/nangate45/bp_fe_top/fakeram45_512x64.lib ./designs/nangate45/bp_fe_top/fakeram45_64x7.lib ./designs/nangate45/bp_fe_top/fakeram45_64x96.lib  
set env KLAYOUT_TECH_FILE ./platforms/nangate45/FreePDK45.lyt
set env TCLLIBPATH util/cell-veneer 
set env ADDITIONAL_LIBS ./designs/nangate45/bp_fe_top/fakeram45_512x64.lib ./designs/nangate45/bp_fe_top/fakeram45_64x7.lib ./designs/nangate45/bp_fe_top/fakeram45_64x96.lib
set env TEST_DIR ./test
set env STREAM_SYSTEM GDS
set env DESIGN_DIR ./designs/nangate45/bp_fe_top/
set env CELL_PAD_IN_SITES_GLOBAL_PLACEMENT 2
set env TIELO_CELL_AND_PORT LOGIC0_X1 Z
set env VARS_BASENAME vars-bp_fe-nangate45-base
set env CELL_PAD_IN_SITES_DETAIL_PLACEMENT 1
set env WRAP_CFG ./platforms/nangate45/wrapper.cfg
set env MACRO_PLACE_HALO 22.4 15.12
set env RULES_DESIGN ./designs/nangate45/bp_fe_top/rules.json
set env DONT_USE_LIBS ./objects/nangate45/bp_fe/base/lib/NangateOpenCellLibrary_typical.lib ./objects/nangate45/bp_fe/base/lib/fakeram45_512x64.lib ./objects/nangate45/bp_fe/base/lib/fakeram45_64x7.lib ./objects/nangate45/bp_fe/base/lib/fakeram45_64x96.lib
set env MAX_FANOUT 100
set env CTS_BUF_CELL BUF_X4
set env OPENROAD_ARGS -no_init 
set env RESULTS_DIR ./results/nangate45/bp_fe/base
set env NPROC 8
set env SYNTH_ARGS -flatten
set env RESYNTH_AREA_RECOVER 0
set env TIME_CMD /usr/bin/time -f Elapsed time: %E[h:]min:sec. Average CPU: %P. Peak memory: %MKB.
set env OPENROAD_NO_EXIT_CMD /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/tools/install/OpenROAD/bin/openroad -no_init 
set env CDL_FILE ./platforms/nangate45/cdl/NangateOpenCellLibrary.cdl
set env TIME_TEST foo
set env KLAYOUT_CMD /home/tool/klayout/install/0.26.5/bin/klayout
set env PUBLIC nangate45 sky130hd sky130hs asap7
set env TEMPLATE_PGA_CFG ./platforms/nangate45/template_pga.cfg
set env WRAPPED_GDSOAS 
set env DIE_AREA 0 0 999.97 799.4
set env DESIGN_NICKNAME bp_fe
set env GDSOAS_FILES ./platforms/nangate45/gds/NangateOpenCellLibrary.gds 
set env KLAYOUT_DRC_FILE ./platforms/nangate45/drc/FreePDK45.lydrc
set env ADDITIONAL_GDSOAS 
set env FILL_CELLS FILLCELL_X1 FILLCELL_X2 FILLCELL_X4 FILLCELL_X8 FILLCELL_X16 FILLCELL_X32
set env DESIGN_HOME ./designs
set env TEST_SCRIPT ./test/core_tests.sh
set env ABC_CLOCK_PERIOD_IN_PS 5400
set env RESULTS_DEF 2_1_floorplan.def 2_2_floorplan_io.def 2_3_floorplan_tdms.def 2_4_floorplan_macro.def 2_5_floorplan_tapcell.def 2_6_floorplan_pdn.def 2_floorplan.def 3_1_place_gp.def 3_2_place_iop.def 3_3_place_resized.def 3_4_place_dp.def 3_place.def
set env WORK_HOME .
set env PRIVATE_DIR ../../private_tool_scripts
set env TIEHI_CELL_AND_PORT LOGIC1_X1 Z
set env PLATFORM_DIR ./platforms/nangate45
set env GDS_FILES ./platforms/nangate45/gds/NangateOpenCellLibrary.gds 
set env SYNTH_HIERARCHICAL 0
set env LSORACLE_PLUGIN /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/tools/install/yosys/share/yosys/plugin/oracle.so
set env SYNTH_STOP_MODULE_SCRIPT ./objects/nangate45/bp_fe/base/mark_hier_stop_modules.tcl
set env ADDER_MAP_FILE ./platforms/nangate45/cells_adders.v
set env LSORACLE_KAHYPAR_CONFIG /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/tools/install/LSOracle/share/lsoracle/test.ini
set env SC_LEF ./platforms/nangate45/lef/NangateOpenCellLibrary.macro.mod.lef
set env TECH_LEF ./platforms/nangate45/lef/NangateOpenCellLibrary.tech.lef
set env FLOW_VARIANT base
set env ADDITIONAL_LEFS ./designs/nangate45/bp_fe_top/fakeram45_512x64.lef ./designs/nangate45/bp_fe_top/fakeram45_64x7.lef ./designs/nangate45/bp_fe_top/fakeram45_64x96.lef  
set env PLACE_DENSITY 0.14
set env MAKEFLAGS 
set env REPORTS_DIR ./reports/nangate45/bp_fe/base
set env FASTROUTE_TCL ./platforms/nangate45/fastroute.tcl
set env SCRIPTS_DIR ./scripts
set env CLKGATE_MAP_FILE ./platforms/nangate45/cells_clkgate.v
set env VERILOG_FILES ./designs/src/bp_fe_top/pickled.v ./designs/nangate45/bp_fe_top/macros.v
set env IO_PLACER_V metal2
set env UTILS_DIR ./util
set env SYNTH_SCRIPT ./scripts/synth.tcl
set env PLACE_PINS_ARGS 
set env 6_1_MERGED_FILE ./results/nangate45/bp_fe/base/6_1_merged.gds
set env PLATFORM nangate45
set env ABC_LOAD_IN_FF 3.898
set env YOSYS_CMD /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/tools/install/yosys/bin/yosys
set env PLACE_SITE FreePDK45_38x28_10R_NP_162NW_34O
set env PROCESS 45
set env MIN_BUF_CELL_AND_PORTS BUF_X1 A Z
set env STREAM_SYSTEM_EXT gds
set env ISSUE_CP_FILE_VARS LATCH_MAP_FILE LIB_FILES SC_LEF TECH_LEF TRACKS_INFO_FILE SDC_FILE VERILOG_FILES TAPCELL_TCL CACHED_NETLIST FOOTPRINT SIG_MAP_FILE PDN_CFG ADDITIONAL_LEFS
set env PLATFORM_HOME ./platforms
set env DESIGN_NAME bp_fe_top
set env MAX_WIRE_LENGTH 1000
set env IO_PLACER_H metal3
set env ABC_DRIVER_CELL BUF_X1
set env CTS_TECH_DIR ./platforms/nangate45/tritonCTS
set env DONT_USE_CELLS TAPCELL_X1 FILLCELL_X1 AOI211_X1 OAI211_X1
set env LSORACLE_CMD /home/zf4_projects/OpenROAD-UFRGS/eder/tap/OpenROAD-flow-scripts/tools/install/LSOracle/bin/lsoracle
set env LOG_DIR ./logs/nangate45/bp_fe/base
set env SEAL_GDSOAS 
set env NUM_CORES 8
set env PDN_CFG ./platforms/nangate45/pdn.cfg
