define_corners fast slow
read_liberty -corner fast  ./../../../test/Nangate45/Nangate45_fast.lib
read_liberty -corner slow ./../../../test/Nangate45/Nangate45_slow.lib
read_lef ./../../../test/Nangate45/Nangate45.lef
read_verilog ./../../../test/gcd_nangate45.v
link_design gcd
read_sdc ./../../../test/timing_api_2.sdc
report_metrics -stage 2 -when final -include_erc false -include_clock_skew false
