set env ABC_AREA 0
set env ABC_DRIVER_CELL BUFx2_ASAP7_75t_R
set env ABC_LOAD_IN_FF 3.898
set env ADDER_MAP_FILE platforms/asap7/yoSys/cells_adders_R.v
set env ADDITIONAL_GDSOAS 
set env ADDITIONAL_LEFS designs/asap7/mock-array/results/asap7/Element/base/Element.lef
set env ADDITIONAL_LIBS designs/asap7/mock-array/results/asap7/Element/base/Element.lib
set env ASAP7_USE_VT RVT
set env BC_CCS_DFF_LIB_FILE platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_FF_ccs_220123.lib
set env BC_CCS_LIB_FILES platforms/asap7/lib/NLDM/asap7sc7p5t_AO_RVT_FF_ccs_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_INVBUF_RVT_FF_ccs_220122.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_OA_RVT_FF_ccs_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SIMPLE_RVT_FF_ccs_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_FF_ccs_220123.lib
set env BC_NLDM_DFF_LIB_FILE platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
set env BC_NLDM_LIB_FILES platforms/asap7/lib/NLDM/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
set env BC_TEMPERATURE 25C
set env BC_VOLTAGE 0.77
set env CELL_PAD_IN_SITES_DETAIL_PLACEMENT 0
set env CELL_PAD_IN_SITES_GLOBAL_PLACEMENT 0
set env CLKGATE_MAP_FILE platforms/asap7/yoSys/cells_clkgate_R.v
set env CORNER BC
set env CTS_CLUSTER_DIAMETER 20
set env CTS_CLUSTER_SIZE 50
set env DB_FILES 
set env DESIGN_CONFIG config.mk
set env DESIGN_DIR ./
set env DESIGN_HOME getenv("FLOW_HOME")/designs
set env DESIGN_NAME MockArray
set env DESIGN_NICKNAME MockArray
set env DETAILED_METRICS 0
set env DETAILED_ROUTE_END_ITERATION 64
set env DONT_USE_CELLS *x1p*_ASAP7* *xp*_ASAP7* SDF* ICG*
set env DONT_USE_LIBS  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/Element.lib
set env DONT_USE_SC_LIB ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/merged.lib
set env DPO_MAX_DISPLACEMENT 5 1
set env ENABLE_DPO 1
set env EQUIVALENCE_CHECK 0
set env FILL_CELLS FILLERxp5_ASAP7_75t_R FILLER_ASAP7_75t_R DECAPx1_ASAP7_75t_R DECAPx2_ASAP7_75t_R DECAPx4_ASAP7_75t_R DECAPx6_ASAP7_75t_R DECAPx10_ASAP7_75t_R
set env FLOW_VARIANT base
set env GDSOAS_FILES platforms/asap7/gds/asap7sc7p5t_28_R_220121a.gds
set env GDS_ALLOW_EMPTY fakeram.*
set env GDS_FILES platforms/asap7/gds/asap7sc7p5t_28_R_220121a.gds
set env GDS_FINAL_FILE ./designs/asap7/mock-array/results/asap7/MockArray/base/6_final.gds
set env GDS_MERGED_FILE ./designs/asap7/mock-array/results/asap7/MockArray/base/6_1_merged.gds
set env GENERATE_ARTIFACTS_ON_FAILURE 1
set env GLOBAL_PLACEMENT_ARGS 
set env GLOBAL_ROUTE_ARGS -congestion_iterations 30 -congestion_report_iter_step 5 -verbose
set env GND_NETS_VOLTAGES VSS 0.0
set env GPL_ROUTABILITY_DRIVEN 1
set env GPL_TIMING_DRIVEN 1
set env GUI_TIMING 1
set env HOLD_BUF_CELL BUFx2_ASAP7_75t_R
set env HOLD_SLACK_MARGIN 0
set env IO_PLACER_H M4
set env IO_PLACER_V M5
set env IR_DROP_LAYER M1
set env LATCH_MAP_FILE platforms/asap7/yoSys/cells_latch_R.v
set env LIB_DIR platforms/asap7/lib/NLDM
set env LIB_FILES platforms/asap7/lib/NLDM/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib designs/asap7/mock-array/results/asap7/Element/base/Element.lib
set env LIB_MODEL NLDM
set env LOG_DIR ./designs/asap7/mock-array/logs/asap7/MockArray/base
set env MACRO_PLACE_HALO 10 10
set env MACRO_ROWS_HALO_X 2
set env MACRO_ROWS_HALO_Y 2
set env MAKE_TRACKS platforms/asap7/openRoad/make_tracks.tcl
set env MATCH_CELL_FOOTPRINT 0
set env MAX_ROUTING_LAYER M9
set env MIN_BUF_CELL_AND_PORTS BUFx2_ASAP7_75t_R A Y
set env MIN_ROUTING_LAYER M2
set env OBJECTS_DIR ./designs/asap7/mock-array/objects/asap7/MockArray/base
set env OPENROAD_ARGS -no_init -threads 48
set env PDN_TCL platforms/asap7/openRoad/pdn/grid_strategy-M1-M2-M5-M6.tcl
set env PLACE_DENSITY 0.60
set env PLACE_PINS_ARGS 
set env PLACE_SITE asap7sc7p5t
set env PLATFORM asap7
set env PLATFORM_DIR platforms/asap7
set env PLATFORM_HOME getenv("FLOW_HOME")/platforms
set env PROCESS 7
set env PWR_NETS_VOLTAGES VDD 0.77
set env RCX_RULES platforms/asap7/rcx_patterns.rules
set env RECOVER_POWER 0
set env REPORTS_DIR ./designs/asap7/mock-array/reports/asap7/MockArray/base
set env REPORT_CLOCK_SKEW 1
set env RESULTS_DEF 
set env RESULTS_DIR ./designs/asap7/mock-array/results/asap7/MockArray/base
set env RESULTS_GDS 
set env RESULTS_OAS 
set env RESYNTH_AREA_RECOVER 0
set env RESYNTH_TIMING_RECOVER 0
set env ROUTING_LAYER_ADJUSTMENT 0.5
set env RTLMP_AREA_WT 0.1
set env RTLMP_BOUNDARY_WT 50.0
set env RTLMP_DEAD_SPACE 0.05
set env RTLMP_FENCE_LX 0.0
set env RTLMP_FENCE_LY 0.0
set env RTLMP_FENCE_UX 100000000.0
set env RTLMP_FENCE_UY 100000000.0
set env RTLMP_MAX_LEVEL 2
set env RTLMP_MIN_AR 0.33
set env RTLMP_NOTCH_WT 10.0
set env RTLMP_OUTLINE_WT 100.0
set env RTLMP_SIGNATURE_NET_THRESHOLD 50
set env RTLMP_WIRELENGTH_WT 100.0
set env RUN_LOG_NAME_STEM run
set env SCRIPTS_DIR ./scripts
set env SC_LEF platforms/asap7/lef/asap7sc7p5t_28_R_1x_220121a.lef
set env SDC_FILE_CLOCK_PERIOD ./designs/asap7/mock-array/results/asap7/MockArray/base/clock_period.txt
set env SEAL_GDSOAS 
set env SETUP_SLACK_MARGIN 0
set env SET_RC_TCL platforms/asap7/setRC.tcl
set env SKIP_INCREMENTAL_REPAIR 0
set env SKIP_REPORT_METRICS 1
set env STREAM_SYSTEM GDS
set env STREAM_SYSTEM_EXT gds
set env SYNTH_ARGS -flatten
set env SYNTH_HIERARCHICAL 0
set env SYNTH_MEMORY_MAX_BITS 4096
set env SYNTH_MINIMUM_KEEP_SIZE 1000
set env SYNTH_SCRIPT getenv("FLOW_HOME")/scripts/synth.tcl
set env TAPCELL_TCL platforms/asap7/openRoad/tapcell.tcl
set env TAP_CELL_NAME TAPCELL_ASAP7_75t_R
set env TC_NLDM_DFF_LIB_FILE platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_TT_nldm_220123.lib
set env TC_NLDM_LIB_FILES platforms/asap7/lib/NLDM/asap7sc7p5t_AO_RVT_TT_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_INVBUF_RVT_TT_nldm_220122.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_OA_RVT_TT_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_TT_nldm_220123.lib platforms/asap7/lib/NLDM/asap7sc7p5t_SIMPLE_RVT_TT_nldm_211120.lib.gz
set env TC_TEMPERATURE 0C
set env TC_VOLTAGE 0.70
set env TECH_LEF platforms/asap7/lef/asap7_tech_1x_201209.lef
set env TEMPERATURE 25C
set env TEST_DIR getenv("FLOW_HOME")/test
set env TIEHI_CELL_AND_PORT TIEHIx1_ASAP7_75t_R H
set env TIELO_CELL_AND_PORT TIELOx1_ASAP7_75t_R L
set env TIME_BIN env time
set env TIME_TEST foo
set env TNS_END_PERCENT 100
set env USE_FILL 0
set env UTILS_DIR getenv("FLOW_HOME")/util
set env VERILOG_TOP_PARAMS 
set env VOLTAGE 0.77
set env VT_TAG R
set env WC_NLDM_DFF_LIB_FILE platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_SS_nldm_220123.lib
set env WC_NLDM_LIB_FILES platforms/asap7/lib/NLDM/asap7sc7p5t_AO_RVT_SS_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_INVBUF_RVT_SS_nldm_220122.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_OA_RVT_SS_nldm_211120.lib.gz platforms/asap7/lib/NLDM/asap7sc7p5t_SEQ_RVT_SS_nldm_220123.lib platforms/asap7/lib/NLDM/asap7sc7p5t_SIMPLE_RVT_SS_nldm_211120.lib.gz
set env WC_TEMPERATURE 100C
set env WC_VOLTAGE 0.63
set env WORK_HOME ./designs/asap7/mock-array
set env WRAPPED_GDSOAS 
set env WRAPPED_LEFS 
set env WRAPPED_LIBS 
set env WRAP_CFG platforms/asap7/wrapper.cfg
set env YOSYS_DEPENDENCIES  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib  ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib ./designs/asap7/mock-array/objects/asap7/MockArray/base/lib/Element.lib     platforms/asap7/yoSys/cells_latch_R.v platforms/asap7/yoSys/cells_adders_R.v ./designs/asap7/mock-array/results/asap7/MockArray/base/clock_period.txt
set env YOSYS_FLAGS -v 3
