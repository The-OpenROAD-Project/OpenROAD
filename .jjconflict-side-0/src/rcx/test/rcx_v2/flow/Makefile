top_dir1=$(shell pwd)
top_dir=$(top_dir1)
#top_dir=/home/dimitris-ic/z/72424/RegressionTest/
data_dir=$(top_dir)/data

TOP_DIR=/home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/
test_dir=$(TOP_DIR)/src/rcx/test/
or_exec=$(TOP_DIR)/build/src/openroad
run_rcx=$(test_dir)/rcx_v2/flow/scripts/run_rcx.bash

.PHONY : gcd_flow_v2_model_v2 gcd_flow_v2_model_v1 gcd_flow_v1_model_v2 ext_patterns_flow_v2_model_v2

.SILENT:

all: options flow 

options: gcd_flow_v2_model_v1_no_via_merge gcd_v2_v1_max_res_0 gcd_v2_v1_max_res_0_no_via_merge gcd_v2_v1_max_res_10 gcd_v2_v1_max_res_500 gcd_v2_lef_rc_max_res_0 gcd_v2_v1_lef_rc_max_res_0

flow: gcd_flow_v2_model_v2 gcd_flow_v2_model_v1 gcd_flow_v1_model_v2 ext_patterns_flow_v2_model_v2

gcd_flow_v2_model_v2: 
			$(run_rcx) $(or_exec) gcd $@

gcd_flow_v2_model_v1:
			$(run_rcx) $(or_exec) gcd $@

gcd_flow_v1_model_v2:
			$(run_rcx) $(or_exec) gcd $@

# gcd_flow_v1_model_v2_1:
			# $(run_rcx) $(or_exec) gcd $@

ext_patterns_flow_v2_model_v2:
			$(run_rcx) $(or_exec) patterns $@

gcd_flow_v2_model_v2_overcell: 
			$(run_rcx) $(or_exec) gcd $@

gcd_flow_v2_model_v1_no_via_merge: 
			$(run_rcx) $(or_exec) options $@

gcd_v2_v1_max_res_0: 
			$(run_rcx) $(or_exec) options $@

gcd_v2_v1_max_res_10: 
			$(run_rcx) $(or_exec) options $@

gcd_v2_v1_max_res_0_no_via_merge: 
			$(run_rcx) $(or_exec) options $@

gcd_v2_v1_max_res_500: 
			$(run_rcx) $(or_exec) options $@

gcd_v2_lef_rc_max_res_0: 
			$(run_rcx) $(or_exec) options $@

gcd_v2_v1_lef_rc_max_res_0: 
			$(run_rcx) $(or_exec) options $@

