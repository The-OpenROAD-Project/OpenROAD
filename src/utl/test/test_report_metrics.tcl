read_liberty ./../../../test/Nangate45/Nangate45_fast.lib
read_lef ./../../../test/Nangate45/Nangate45.lef
read_verilog ./../../../test/gcd_nangate45.v
link_design gcd
read_sdc ./../../../test/timing_api_2.sdc
report_metrics 2 final