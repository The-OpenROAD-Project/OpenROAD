# Tests that skipping early iterations min overflow updating produces
# convergence with -timing_driven (see NesterovPlace::doNesterovPlace)

source helpers.tcl
set test_name convergence01

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_def convergence01.def

read_sdc convergence01.sdc

source asap7/setRC.tcl

global_placement -density 0.5 -timing_driven \
    -pad_left 2 \
    -pad_right 2

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
