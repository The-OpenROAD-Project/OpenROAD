# PARALLEL_EVAL VtSwapMt with a liberty per corner.  The worst paths are in
# the slow corner, whose timing arcs hold the cells of its own libraries.
source "helpers.tcl"
# Both corners read the same libraries.
suppress_message STA 1140
suppress_message STA 1212

define_corners fast slow
foreach corner {fast slow} {
  foreach vt {RVT LVT SLVT} {
    read_liberty -corner $corner asap7/asap7sc7p5t_AO_${vt}_FF_nldm_211120.lib.gz
    read_liberty -corner $corner \
      asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
    read_liberty -corner $corner asap7/asap7sc7p5t_OA_${vt}_FF_nldm_211120.lib.gz
    read_liberty -corner $corner \
      asap7/asap7sc7p5t_SIMPLE_${vt}_FF_nldm_211120.lib.gz
    read_liberty -corner $corner asap7/asap7sc7p5t_SEQ_${vt}_FF_nldm_220123.lib
  }
}

read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def
read_sdc gcd.sdc
source asap7/setRC.tcl
set_wire_rc -corner slow -signal -resistance 6.46302E-02 -capacitance 3.46646E-01
estimate_parasitics -placement

repair_timing -setup \
  -phases PARALLEL_EVAL \
  -sequence vt_swap \
  -max_iterations 2 \
  -verbose
