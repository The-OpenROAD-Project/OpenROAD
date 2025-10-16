top_dir1=$(shell pwd)
top_dir=$(top_dir1)
#top_dir=/home/dimitris-ic/z/72424/RegressionTest/
data_dir=$(top_dir)/data

TOP_DIR=/home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/
test_dir=$(TOP_DIR)/src/rcx/test/
or_exec=$(TOP_DIR)/build/src/openroad
run_rcx_corners=$(test_dir)/rcx_v2/flow/corners/scripts/run_rcx.bash

.PHONY : gcd_3corners gcd_3corners_define_list gcd_3corners_undefined  gcd_corners_0  gcd_corners_0_2  gcd_corners_1  gcd_corners_2  gcd_corners_2_1

.SILENT:

all: gcd_3corners gcd_3corners_define_list gcd_3corners_undefined  gcd_corners_0  gcd_corners_0_2  gcd_corners_1  gcd_corners_2  gcd_corners_2_1

v1_gcd_3corners_define_list: 
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_3corners: 
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_3corners_define_list:  
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_3corners_undefined:  
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_corners_0:  
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_corners_0_2:  
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_corners_1:  
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_corners_2:  
			$(run_rcx_corners) $(or_exec) ./ $@

gcd_corners_2_1:
			$(run_rcx_corners) $(or_exec) ./ $@

