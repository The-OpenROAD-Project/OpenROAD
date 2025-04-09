# Sets up ORFS variables using make variable support, relying
# on makefile features such as defaults, forward references,
# lazy evaluation, conditional code, include statements,
# etc.

# Setup variables to point to root / head of the OpenROAD directory
# - the following settings allowed user to point OpenROAD binaries to different
#   location
# - default is current install / clone directory
ifeq ($(origin FLOW_HOME), undefined)
FLOW_HOME := $(abspath $(dir $(firstword $(MAKEFILE_LIST)))/..)
endif
export FLOW_HOME

export DESIGN_NICKNAME?=$(DESIGN_NAME)

#-------------------------------------------------------------------------------
# Setup variables to point to other location for the following sub directory
# - designs - default is under current directory
# - platforms - default is under current directory
# - work home - default is current directory
# - utils, scripts, test - default is under current directory
export DESIGN_HOME   ?= $(FLOW_HOME)/designs
export PLATFORM_HOME ?= $(FLOW_HOME)/platforms
export WORK_HOME     ?= .

export UTILS_DIR     ?= $(FLOW_HOME)/util
export SCRIPTS_DIR   ?= $(FLOW_HOME)/scripts
export TEST_DIR      ?= $(FLOW_HOME)/test

PUBLIC=nangate45 sky130hd sky130hs asap7 ihp-sg13g2 gf180

ifeq ($(origin PLATFORM), undefined)
  $(error PLATFORM variable net set.)
endif
ifeq ($(origin DESIGN_NAME), undefined)
  $(error DESIGN_NAME variable net set.)
endif

ifneq ($(PLATFORM_DIR),)
else ifneq ($(wildcard $(PLATFORM_HOME)/$(PLATFORM)),)
  export PLATFORM_DIR = $(PLATFORM_HOME)/$(PLATFORM)
else ifneq ($(findstring $(PLATFORM),$(PUBLIC)),)
  export PLATFORM_DIR = ./platforms/$(PLATFORM)
else ifneq ($(wildcard ../../$(PLATFORM)),)
  export PLATFORM_DIR = ../../$(PLATFORM)
else
  $(error [ERROR][FLOW] Platform '$(PLATFORM)' not found.)
endif

include $(PLATFORM_DIR)/config.mk

# __SPACE__ is a workaround for whitespace hell in "foreach"; there
# is no way to escape space in defaults.py and get "foreach" to work.
$(foreach line,$(shell $(SCRIPTS_DIR)/defaults.py),$(eval export $(subst __SPACE__, ,$(line))))

export LOG_DIR     = $(WORK_HOME)/logs/$(PLATFORM)/$(DESIGN_NICKNAME)/$(FLOW_VARIANT)
export OBJECTS_DIR = $(WORK_HOME)/objects/$(PLATFORM)/$(DESIGN_NICKNAME)/$(FLOW_VARIANT)
export REPORTS_DIR = $(WORK_HOME)/reports/$(PLATFORM)/$(DESIGN_NICKNAME)/$(FLOW_VARIANT)
export RESULTS_DIR = $(WORK_HOME)/results/$(PLATFORM)/$(DESIGN_NICKNAME)/$(FLOW_VARIANT)

#-------------------------------------------------------------------------------
ifeq (,$(strip $(NUM_CORES)))
  # Linux (utility program)
  NUM_CORES := $(shell nproc 2>/dev/null)

  ifeq (,$(strip $(NUM_CORES)))
    # Linux (generic)
    NUM_CORES := $(shell grep -c ^processor /proc/cpuinfo 2>/dev/null)
  endif
  ifeq (,$(strip $(NUM_CORES)))
    # BSD (at least FreeBSD and Mac OSX)
    NUM_CORES := $(shell sysctl -n hw.ncpu 2>/dev/null)
  endif
  ifeq (,$(strip $(NUM_CORES)))
    # Fallback
    NUM_CORES := 1
  endif
endif
export NUM_CORES

#-------------------------------------------------------------------------------
# setup all commands used within this flow
export TIME_BIN   ?= env time
TIME_CMD = $(TIME_BIN) -f 'Elapsed time: %E[h:]min:sec. CPU time: user %U sys %S (%P). Peak memory: %MKB.'
TIME_TEST = $(shell $(TIME_CMD) echo foo 2>/dev/null)
ifeq (,$(strip $(TIME_TEST)))
  TIME_CMD = $(TIME_BIN)
endif
export TIME_CMD

# The following determine the executable location for each tool used by this flow.
# Priority is given to
#       1 user explicit set with variable in Makefile or command line, for instance setting OPENROAD_EXE
#       2 ORFS compiled tools: openroad, yosys
ifneq (${IN_NIX_SHELL},)
  export OPENROAD_EXE := $(shell command -v openroad)
else
  export OPENROAD_EXE ?= $(abspath $(FLOW_HOME)/../tools/install/OpenROAD/bin/openroad)
endif
ifneq (${IN_NIX_SHELL},)
  export OPENSTA_EXE := $(shell command -v sta)
else
  export OPENSTA_EXE ?= $(abspath $(FLOW_HOME)/../tools/install/OpenROAD/bin/sta)
endif

export OPENROAD_ARGS = -no_init -threads $(NUM_CORES) $(OR_ARGS)
export OPENROAD_CMD = $(OPENROAD_EXE) -exit $(OPENROAD_ARGS)
export OPENROAD_NO_EXIT_CMD = $(OPENROAD_EXE) $(OPENROAD_ARGS)
export OPENROAD_GUI_CMD = $(OPENROAD_EXE) -gui $(OR_ARGS)

ifneq (${IN_NIX_SHELL},)
  YOSYS_EXE := $(shell command -v yosys)
else
  YOSYS_EXE ?= $(abspath $(FLOW_HOME)/../tools/install/yosys/bin/yosys)
endif
export YOSYS_EXE

# Use locally installed and built klayout if it exists, otherwise use klayout in path
KLAYOUT_DIR = $(abspath $(FLOW_HOME)/../tools/install/klayout/)
KLAYOUT_BIN_FROM_DIR = $(KLAYOUT_DIR)/klayout

ifeq ($(wildcard $(KLAYOUT_BIN_FROM_DIR)), $(KLAYOUT_BIN_FROM_DIR))
KLAYOUT_CMD ?= sh -c 'LD_LIBRARY_PATH=$(dir $(KLAYOUT_BIN_FROM_DIR)) $$0 "$$@"' $(KLAYOUT_BIN_FROM_DIR)
else
ifeq ($(KLAYOUT_CMD),)
KLAYOUT_CMD := $(shell command -v klayout)
endif
endif
KLAYOUT_FOUND            = $(if $(KLAYOUT_CMD),,$(error KLayout not found in PATH))

ifneq ($(shell command -v stdbuf),)
  STDBUF_CMD ?= stdbuf -o L
endif

#-------------------------------------------------------------------------------
WRAPPED_LEFS = $(foreach lef,$(notdir $(WRAP_LEFS)),$(OBJECTS_DIR)/lef/$(lef:.lef=_mod.lef))
WRAPPED_LIBS = $(foreach lib,$(notdir $(WRAP_LIBS)),$(OBJECTS_DIR)/$(lib:.lib=_mod.lib))
export ADDITIONAL_LEFS += $(WRAPPED_LEFS) $(WRAP_LEFS)
export LIB_FILES += $(WRAP_LIBS) $(WRAPPED_LIBS)

export DONT_USE_LIBS   = $(patsubst %.lib.gz, %.lib, $(addprefix $(OBJECTS_DIR)/lib/, $(notdir $(LIB_FILES))))
export DONT_USE_SC_LIB ?= $(firstword $(DONT_USE_LIBS))

# Stream system used for final result (GDS is default): GDS, GSDII, GDS2, OASIS, or OAS
STREAM_SYSTEM ?= GDS
ifneq ($(findstring GDS,$(shell echo $(STREAM_SYSTEM) | tr '[:lower:]' '[:upper:]')),)
	export STREAM_SYSTEM_EXT := gds
	GDSOAS_FILES = $(GDS_FILES)
	ADDITIONAL_GDSOAS = $(ADDITIONAL_GDS)
	SEAL_GDSOAS = $(SEAL_GDS)
else
	export STREAM_SYSTEM_EXT := oas
	GDSOAS_FILES = $(OAS_FILES)
	ADDITIONAL_GDSOAS = $(ADDITIONAL_OAS)
	SEAL_GDSOAS = $(SEAL_OAS)
endif
export WRAPPED_GDSOAS = $(foreach lef,$(notdir $(WRAP_LEFS)),$(OBJECTS_DIR)/$(lef:.lef=_mod.$(STREAM_SYSTEM_EXT)))

# If we are running headless use offscreen rendering for save_image
ifeq ($(DISPLAY),)
export QT_QPA_PLATFORM ?= offscreen
endif

# Create Macro wrappers (if necessary)
export WRAP_CFG = $(PLATFORM_DIR)/wrapper.cfg

export TCLLIBPATH := util/cell-veneer $(TCLLIBPATH)

export SYNTH_SCRIPT ?= $(SCRIPTS_DIR)/synth.tcl
export SDC_FILE_CLOCK_PERIOD = $(RESULTS_DIR)/clock_period.txt

export YOSYS_DEPENDENCIES=$(DONT_USE_LIBS) $(WRAPPED_LIBS) $(DFF_LIB_FILE) $(VERILOG_FILES) $(SYNTH_NETLIST_FILES) $(LATCH_MAP_FILE) $(ADDER_MAP_FILE) $(SDC_FILE_CLOCK_PERIOD)

# Ubuntu 22.04 ships with older than 0.28.11, so support older versions
# for a while still.
export KLAYOUT_ENV_VAR_IN_PATH_VERSION = 0.28.11
export KLAYOUT_VERSION := $(if $(KLAYOUT_CMD),$(shell $(KLAYOUT_CMD) -v 2>/dev/null | grep 'KLayout' | cut -d ' ' -f2),)

export KLAYOUT_ENV_VAR_IN_PATH = $(shell \
	if [ -z "$(KLAYOUT_VERSION)" ]; then \
		echo "not_found"; \
	elif [ "$$(echo -e "$(KLAYOUT_VERSION)\n$(KLAYOUT_ENV_VAR_IN_PATH_VERSION)" | sort -V | head -n1)" = "$(KLAYOUT_VERSION)" ] && [ "$(KLAYOUT_VERSION)" != "$(KLAYOUT_ENV_VAR_IN_PATH_VERSION)" ]; then \
		echo "invalid"; \
	else \
		echo "valid"; \
	fi)

export GDS_FINAL_FILE = $(RESULTS_DIR)/6_final.$(STREAM_SYSTEM_EXT)
export RESULTS_ODB = $(notdir $(sort $(wildcard $(RESULTS_DIR)/*.odb)))
export RESULTS_DEF = $(notdir $(sort $(wildcard $(RESULTS_DIR)/*.def)))
export RESULTS_GDS = $(notdir $(sort $(wildcard $(RESULTS_DIR)/*.gds)))
export RESULTS_OAS = $(notdir $(sort $(wildcard $(RESULTS_DIR)/*.oas)))
export GDS_MERGED_FILE = $(RESULTS_DIR)/6_1_merged.$(STREAM_SYSTEM_EXT)

define get_variables
$(foreach V, $(.VARIABLES),$(if $(filter-out $(1), $(origin $V)), $(if $(filter-out .% %QT_QPA_PLATFORM% %TIME_CMD% KLAYOUT% GENERATE_ABSTRACT_RULE% do-step% do-copy% OPEN_GUI% OPEN_GUI_SHORTCUT% SUB_MAKE% UNSET_VARS% export%, $(V)), $V$ )))
endef

export UNSET_VARIABLES_NAMES := $(call get_variables,command% line environment% default automatic)
export ISSUE_VARIABLES_NAMES := $(sort $(filter-out \n get_variables, $(call get_variables,environment% default automatic)))
# This is Makefile's way to define a macro that expands to a single newline.
define newline


endef
export ISSUE_VARIABLES := $(foreach V, $(ISSUE_VARIABLES_NAMES), $(if $($V),$V=$($V),$V='')$(newline))
export COMMAND_LINE_ARGS := $(foreach V,$(.VARIABLES),$(if $(filter command% line, $(origin $V)),$(V)))

# Set yosys-abc clock period to first "clk_period" value or "-period" value found in sdc file
ifeq ($(origin ABC_CLOCK_PERIOD_IN_PS), undefined)
   ifneq ($(wildcard $(SDC_FILE)),)
      export ABC_CLOCK_PERIOD_IN_PS := $(shell sed -nE "s/^set\s+clk_period\s+(\S+).*|.*-period\s+(\S+).*/\1\2/p" $(SDC_FILE) | head -1 | awk '{print $$1}')
   endif
endif

.PHONY: vars
vars:
	mkdir -p $(OBJECTS_DIR)
	$(UTILS_DIR)/generate-vars.sh $(OBJECTS_DIR)/vars

.PHONY: print-%
# Print any variable, for instance: make print-DIE_AREA
print-%  : ; @echo "$* = $($*)"
