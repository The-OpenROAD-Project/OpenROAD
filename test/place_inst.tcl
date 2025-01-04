# check accessors for die and core area in microns
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_liberty Nangate45/Nangate45_typ.lib

read_verilog gcd_nangate45.v
link_design gcd

initialize_floorplan \
    -site FreePDK45_38x28_10R_NP_162NW_34O \
    -die_area {0 0 500 500} \
    -core_area {5 5 490 490}

place_inst \
    -name test0 \
    -location "50 50" \
    -cell HM_100x100_1x1_CENTERED \
    -status FIRM

place_inst \
    -name test1 \
    -origin "200 100" \
    -cell HM_100x100_1x1_CENTERED

catch {place_inst \
        -name test2 \
        -cell HM_100x100_1x1_CENTERED} msg
puts $msg

catch {place_inst \
        -name test2 \
        -origin "200 200" \
        -location "50 50" \
        -cell HM_100x100_1x1_CENTERED} msg
puts $msg

catch {place_inst \
        -name test2 \
        -origin "200 200"} msg
puts $msg

place_inst \
    -name test2 \
    -origin "300 100" \
    -orientation R180 \
    -cell HM_100x100_1x1_CENTERED

place_inst \
    -name test3 \
    -origin "150 400" \
    -orientation R180 \
    -cell HM_100x100_1x1

place_inst \
    -name test4 \
    -location "250 350" \
    -orientation R180 \
    -cell HM_100x100_1x1

set def_file [make_result_file place_inst.def]
write_def $def_file
diff_file $def_file place_inst.defok
