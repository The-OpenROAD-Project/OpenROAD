export PLATFORM                = asap7
export PROCESS                 = 7
export ASAP7_USE_VT           ?= RVT

ifeq ($(LIB_MODEL),)
   export LIB_MODEL = NLDM
endif
export LIB_DIR                ?= $(PLATFORM_DIR)/lib/$(LIB_MODEL)

#Library Setup variable
export TECH_LEF                = $(PLATFORM_DIR)/lef/asap7_tech_1x_201209.lef

export BC_TEMPERATURE          = 25C
export TC_TEMPERATURE          = 0C
export WC_TEMPERATURE          = 100C

export BC_VOLTAGE          = 0.77
export TC_VOLTAGE          = 0.70
export WC_VOLTAGE          = 0.63

# Dont use cells to ease congestion
# Specify at least one filler cell if none
export DONT_USE_CELLS          = *x1p*_ASAP7* *xp*_ASAP7*
export DONT_USE_CELLS          += SDF* ICG*

export SYNTH_MINIMUM_KEEP_SIZE       ?= 1000

# BUF_X1, pin (A) = 0.974659. Arbitrarily multiply by 4
export ABC_LOAD_IN_FF          = 3.898


# Placement site for core cells
# This can be found in the technology lef
export PLACE_SITE              ?= asap7sc7p5t

export MAKE_TRACKS             = $(PLATFORM_DIR)/openRoad/make_tracks.tcl

# Define default PDN config
ifeq ($(BLOCKS),)
   export PDN_TCL ?= $(PLATFORM_DIR)/openRoad/pdn/grid_strategy-M1-M2-M5-M6.tcl
else
   export PDN_TCL ?= $(PLATFORM_DIR)/openRoad/pdn/BLOCKS_grid_strategy.tcl
endif


# IO Placer pin layers
export IO_PLACER_H             ?= M4
export IO_PLACER_V             ?= M5

export MACRO_PLACE_HALO        ?= 10 10

# the followings create a keep out / halo between
# macro and core rows
export MACRO_ROWS_HALO_X            ?= 2
export MACRO_ROWS_HALO_Y            ?= 2

export PLACE_DENSITY ?= 0.60

# Endcap and Welltie cells
export TAPCELL_TCL             ?= $(PLATFORM_DIR)/openRoad/tapcell.tcl

export SET_RC_TCL              = $(PLATFORM_DIR)/setRC.tcl

# Route options
export MIN_ROUTING_LAYER       ?= M2
#export MIN_CLOCK_ROUTING_LAYER = M4
export MAX_ROUTING_LAYER       ?= M7

# KLayout technology file
export KLAYOUT_TECH_FILE       = $(PLATFORM_DIR)/KLayout/asap7.lyt

# KLayout DRC ruledeck
export KLAYOUT_DRC_FILE = $(PLATFORM_DIR)/drc/asap7.lydrc

# OpenRCX extRules
export RCX_RULES               = $(PLATFORM_DIR)/rcx_patterns.rules

# XS - defining function for using LVT
ifeq ($(ASAP7_USE_VT), LVT)
   export VT_TAG = L
else ifeq ($(ASAP7_USE_VT), SLVT)
   export VT_TAG = SL
else
   # Default to RVT
   export VT_TAG = R
endif

# Set the TIEHI/TIELO cells
# These are used in yosys synthesis to avoid logical 1/0's in the netlist
export TIEHI_CELL_AND_PORT     ?= TIEHIx1_ASAP7_75t_$(VT_TAG) H
export TIELO_CELL_AND_PORT     ?= TIELOx1_ASAP7_75t_$(VT_TAG) L

# Used in synthesis
export MIN_BUF_CELL_AND_PORTS  ?= BUFx2_ASAP7_75t_$(VT_TAG) A Y

export HOLD_BUF_CELL           ?= BUFx2_ASAP7_75t_$(VT_TAG)

export ABC_DRIVER_CELL         ?= BUFx2_ASAP7_75t_$(VT_TAG)

# Fill cells used in fill cell insertion
export FILL_CELLS              ?= FILLERxp5_ASAP7_75t_$(VT_TAG) \
                                  FILLER_ASAP7_75t_$(VT_TAG) \
                                  DECAPx1_ASAP7_75t_$(VT_TAG) \
                                  DECAPx2_ASAP7_75t_$(VT_TAG) \
                                  DECAPx4_ASAP7_75t_$(VT_TAG) \
                                  DECAPx6_ASAP7_75t_$(VT_TAG) \
                                  DECAPx10_ASAP7_75t_$(VT_TAG)

export TAP_CELL_NAME	       ?= TAPCELL_ASAP7_75t_$(VT_TAG)

# GDS_FILES has to be = vs. ?= because GDS_FILES gets set in the ORFS Makefile
export GDS_FILES               = $(PLATFORM_DIR)/gds/asap7sc7p5t_28_$(VT_TAG)_220121a.gds \
			         $(ADDITIONAL_GDS)

export SC_LEF                  ?= $(PLATFORM_DIR)/lef/asap7sc7p5t_28_$(VT_TAG)_1x_220121a.lef

# Yosys mapping files
export LATCH_MAP_FILE          ?= $(PLATFORM_DIR)/yoSys/cells_latch_$(VT_TAG).v
export CLKGATE_MAP_FILE        ?= $(PLATFORM_DIR)/yoSys/cells_clkgate_$(VT_TAG).v
export ADDER_MAP_FILE          ?= $(PLATFORM_DIR)/yoSys/cells_adders_$(VT_TAG).v

export BC_NLDM_DFF_LIB_FILE    ?= $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_FF_nldm_220123.lib

export BC_NLDM_LIB_FILES       ?= $(LIB_DIR)/asap7sc7p5t_AO_$(VT_TAG)VT_FF_nldm_211120.lib.gz \
			          $(LIB_DIR)/asap7sc7p5t_INVBUF_$(VT_TAG)VT_FF_nldm_220122.lib.gz \
			          $(LIB_DIR)/asap7sc7p5t_OA_$(VT_TAG)VT_FF_nldm_211120.lib.gz \
			          $(LIB_DIR)/asap7sc7p5t_SIMPLE_$(VT_TAG)VT_FF_nldm_211120.lib.gz \
			          $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_FF_nldm_220123.lib

export BC_CCS_LIB_FILES        ?= $(LIB_DIR)/asap7sc7p5t_AO_$(VT_TAG)VT_FF_ccs_211120.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_INVBUF_$(VT_TAG)VT_FF_ccs_220122.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_OA_$(VT_TAG)VT_FF_ccs_211120.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_SIMPLE_$(VT_TAG)VT_FF_ccs_211120.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_FF_ccs_220123.lib \
				  $(BC_ADDITIONAL_LIBS)

export BC_CCS_DFF_LIB_FILE     ?= $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_FF_ccs_220123.lib

export WC_NLDM_DFF_LIB_FILE    ?= $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_SS_nldm_220123.lib

export WC_NLDM_LIB_FILES       ?= $(LIB_DIR)/asap7sc7p5t_AO_$(VT_TAG)VT_SS_nldm_211120.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_INVBUF_$(VT_TAG)VT_SS_nldm_220122.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_OA_$(VT_TAG)VT_SS_nldm_211120.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_SS_nldm_220123.lib \
				  $(LIB_DIR)/asap7sc7p5t_SIMPLE_$(VT_TAG)VT_SS_nldm_211120.lib.gz

export TC_NLDM_DFF_LIB_FILE    ?= $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_TT_nldm_220123.lib

export TC_NLDM_LIB_FILES       ?= $(LIB_DIR)/asap7sc7p5t_AO_$(VT_TAG)VT_TT_nldm_211120.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_INVBUF_$(VT_TAG)VT_TT_nldm_220122.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_OA_$(VT_TAG)VT_TT_nldm_211120.lib.gz \
				  $(LIB_DIR)/asap7sc7p5t_SEQ_$(VT_TAG)VT_TT_nldm_220123.lib \
				  $(LIB_DIR)/asap7sc7p5t_SIMPLE_$(VT_TAG)VT_TT_nldm_211120.lib.gz

ifeq ($(CLUSTER_FLOPS),1)
   # Add the multi-bit FF for clustering.  These are single corner libraries.
   export ADDITIONAL_LIBS      += $(LIB_DIR)/asap7sc7p5t_DFFHQNH2V2X_$(VT_TAG)VT_TT_nldm_FAKE.lib \
                                  $(LIB_DIR)/asap7sc7p5t_DFFHQNV2X_$(VT_TAG)VT_TT_nldm_FAKE.lib

   export ADDITIONAL_LEFS      += $(PLATFORM_DIR)/lef/asap7sc7p5t_DFFHQNH2V2X.lef \
                                  $(PLATFORM_DIR)/lef/asap7sc7p5t_DFFHQNV2X.lef
   export ADDITIONAL_SITES     += asap7sc7p5t_pg
   export GDS_ALLOW_EMPTY      ?= DFFHQN[VH][24].*
endif

# Dont use SC library based on CORNER selection
#
# BC - Best case, fastest
# WC - Worst case, slowest
# TC - Typical case
export CORNER ?= BC
export LIB_FILES             += $($(CORNER)_$(LIB_MODEL)_LIB_FILES)
export LIB_FILES             += $(ADDITIONAL_LIBS)
export DB_FILES              += $(realpath $($(CORNER)_DB_FILES))
export TEMPERATURE            = $($(CORNER)_TEMPERATURE)
export VOLTAGE                = $($(CORNER)_VOLTAGE)

# FIXME Need merged.lib for now, but ideally it shouldn't be necessary:
#
# https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/pull/2139
export DONT_USE_SC_LIB        = $(OBJECTS_DIR)/lib/merged.lib

# ---------------------------------------------------------
#  IR Drop
# ---------------------------------------------------------

# IR drop estimation supply net name to be analyzed and supply voltage variable
# For multiple nets: PWR_NETS_VOLTAGES  = "VDD1 1.8 VDD2 1.2"
export PWR_NETS_VOLTAGES  ?= VDD $(VOLTAGE)
export GND_NETS_VOLTAGES  ?= VSS 0.0
export IR_DROP_LAYER ?= M1

# Allow empty GDS cell
export GDS_ALLOW_EMPTY ?= fakeram.*
