source "helpers.tcl"

read_lef NangateOpenCellLibrary.mod.lef
read_lef flipchip_test.dummy_pads.lef

read_liberty dummy_pads.lib

read_verilog flipchip_test/flipchip_test.v

link_design test

if {[catch {ICeWall load_footprint flipchip_test/flipchip_test.package.strategy} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

initialize_floorplan \
  -die_area  {0 0 4000.000 4000.000} \
  -core_area {180.012 180.096 3819.964 3819.712} \
  -site FreePDK45_38x28_10R_NP_162NW_34O

make_tracks

source ../../../test/Nangate45/Nangate45.tracks


ICeWall load_library flipchip_test/flipchip_test.library.strategy

ICeWall set_type Flipchip

ICeWall set_die_area {0 0 4000.000 4000.000}
ICeWall set_core_area {180.012 180.096 3819.964 3819.712}

ICeWall add_power_nets  "VDD DVDD_0 DVDD_1"
ICeWall add_ground_nets "VSS DVSS_0 DVSS_1"

ICeWall set_offsets 55
ICeWall set_pin_layer metal10
  
ICeWall set_pad_inst_name "u_%s"
ICeWall set_pad_pin_name "%s"

ICeWall set_rdl_cover_file_name results/cover.def

ICeWall add_pad -signal sig1 -type sig -location {origin {x 198.5 y 55}} -bump {row 24 col 1}
ICeWall add_pad -signal sig2 -type sig -location {origin {x 353.5 y 55}} -bump {row 24 col 2}
ICeWall add_pad -signal sig3 -type sig -location {origin {x 301.5 y 55}} -bump {row 23 col 2}
ICeWall add_pad -signal sig4 -type sig -location {origin {x 508.5 y 55}} -bump {row 24 col 3}
ICeWall add_pad -signal sig5 -type sig -location {origin {x 456.5 y 55}} -bump {row 23 col 3}
ICeWall add_pad -signal sig250  -type sig -location {origin {x 391.5 y 55}} -bump {row 22 col 2}
ICeWall add_pad -signal sig6 -type sig -location {origin {x 560.5 y 55}} -bump {row 22 col 3}
ICeWall add_pad -signal sig7 -type sig -location {origin {x 663.5 y 55}} -bump {row 24 col 4}
ICeWall add_pad -signal sig8 -type sig -location {origin {x 611.5 y 55}} -bump {row 23 col 4}
ICeWall add_pad -signal sig9 -type sig -location {origin {x 715.5 y 55}} -bump {row 22 col 4}
ICeWall add_pad -signal sig10 -type sig -location {origin {x 818.5 y 55}} -bump {row 24 col 5}
ICeWall add_pad -signal sig11 -type sig -location {origin {x 766.5 y 55}} -bump {row 23 col 5}
ICeWall add_pad -signal sig12 -type sig -location {origin {x 870.5 y 55}} -bump {row 22 col 5}
ICeWall add_pad -signal sig13 -type sig -location {origin {x 973.5 y 55}} -bump {row 24 col 6}
ICeWall add_pad -signal sig14 -type sig -location {origin {x 921.5 y 55}} -bump {row 23 col 6}
ICeWall add_pad -signal sig15 -type sig -location {origin {x 1025.5 y 55}} -bump {row 22 col 6}
ICeWall add_pad -signal sig16 -type sig -location {origin {x 1128.5 y 55}} -bump {row 24 col 7}
ICeWall add_pad -signal sig17 -type sig -location {origin {x 1076.5 y 55}} -bump {row 23 col 7}
ICeWall add_pad -signal sig18 -type sig -location {origin {x 1180.5 y 55}} -bump {row 22 col 7}
ICeWall add_pad -signal sig19 -type sig -location {origin {x 1283.5 y 55}} -bump {row 24 col 8}
ICeWall add_pad -signal sig20 -type sig -location {origin {x 1231.5 y 55}} -bump {row 23 col 8}
ICeWall add_pad -signal sig21 -type sig -location {origin {x 1335.5 y 55}} -bump {row 22 col 8}
ICeWall add_pad -signal sig22 -type sig -location {origin {x 1438.5 y 55}} -bump {row 24 col 9}
ICeWall add_pad -signal sig23 -type sig -location {origin {x 1386.5 y 55}} -bump {row 23 col 9}
ICeWall add_pad -signal sig24 -type sig -location {origin {x 1490.5 y 55}} -bump {row 22 col 9}
ICeWall add_pad -signal sig25 -type sig -location {origin {x 1593.5 y 55}} -bump {row 24 col 10}
ICeWall add_pad -signal sig26 -type sig -location {origin {x 1541.5 y 55}} -bump {row 23 col 10}
ICeWall add_pad -signal sig27 -type sig -location {origin {x 1645.5 y 55}} -bump {row 22 col 10}
ICeWall add_pad -signal sig28 -type sig -location {origin {x 1748.5 y 55}} -bump {row 24 col 11}
ICeWall add_pad -signal sig29 -type sig -location {origin {x 1696.5 y 55}} -bump {row 23 col 11}
ICeWall add_pad -signal sig30 -type sig -location {origin {x 1800.5 y 55}} -bump {row 22 col 11}
ICeWall add_pad -signal sig31 -type sig -location {origin {x 1903.5 y 55}} -bump {row 24 col 12}
ICeWall add_pad -signal sig32 -type sig -location {origin {x 1851.5 y 55}} -bump {row 23 col 12}
ICeWall add_pad -signal sig33 -type sig -location {origin {x 1955.5 y 55}} -bump {row 22 col 12}
ICeWall add_pad -signal sig34 -type sig -location {origin {x 2058.5 y 55}} -bump {row 24 col 13}
ICeWall add_pad -signal sig35 -type sig -location {origin {x 2006.5 y 55}} -bump {row 23 col 13}
ICeWall add_pad -signal sig36 -type sig -location {origin {x 2110.5 y 55}} -bump {row 22 col 13}
ICeWall add_pad -signal sig37 -type sig -location {origin {x 2213.5 y 55}} -bump {row 24 col 14}
ICeWall add_pad -signal sig38 -type sig -location {origin {x 2161.5 y 55}} -bump {row 23 col 14}
ICeWall add_pad -signal sig39 -type sig -location {origin {x 2265.5 y 55}} -bump {row 22 col 14}
ICeWall add_pad -signal sig40 -type sig -location {origin {x 2368.5 y 55}} -bump {row 24 col 15}
ICeWall add_pad -signal sig41 -type sig -location {origin {x 2316.5 y 55}} -bump {row 23 col 15}
ICeWall add_pad -signal sig42 -type sig -location {origin {x 2420.5 y 55}} -bump {row 22 col 15}
ICeWall add_pad -signal sig43 -type sig -location {origin {x 2523.5 y 55}} -bump {row 24 col 16}
ICeWall add_pad -signal sig44 -type sig -location {origin {x 2471.5 y 55}} -bump {row 23 col 16}
ICeWall add_pad -signal sig45 -type sig -location {origin {x 2575.5 y 55}} -bump {row 22 col 16}
ICeWall add_pad -signal sig46 -type sig -location {origin {x 2678.5 y 55}} -bump {row 24 col 17}
ICeWall add_pad -signal sig47 -type sig -location {origin {x 2626.5 y 55}} -bump {row 23 col 17}
ICeWall add_pad -signal sig48 -type sig -location {origin {x 2730.5 y 55}} -bump {row 22 col 17}
ICeWall add_pad -signal sig49 -type sig -location {origin {x 2833.5 y 55}} -bump {row 24 col 18}
ICeWall add_pad -signal sig50 -type sig -location {origin {x 2781.5 y 55}} -bump {row 23 col 18}
ICeWall add_pad -signal sig51 -type sig -location {origin {x 2885.5 y 55}} -bump {row 22 col 18}
ICeWall add_pad -signal sig52 -type sig -location {origin {x 2988.5 y 55}} -bump {row 24 col 19}
ICeWall add_pad -signal sig53 -type sig -location {origin {x 2936.5 y 55}} -bump {row 23 col 19}
ICeWall add_pad -signal sig54 -type sig -location {origin {x 3040.5 y 55}} -bump {row 22 col 19}
ICeWall add_pad -signal sig55 -type sig -location {origin {x 3143.5 y 55}} -bump {row 24 col 20}
ICeWall add_pad -signal sig56 -type sig -location {origin {x 3091.5 y 55}} -bump {row 23 col 20}
ICeWall add_pad -signal sig57 -type sig -location {origin {x 3195.5 y 55}} -bump {row 22 col 20}
ICeWall add_pad -signal sig58 -type sig -location {origin {x 3298.5 y 55}} -bump {row 24 col 21}
ICeWall add_pad -signal sig59 -type sig -location {origin {x 3246.5 y 55}} -bump {row 23 col 21}
ICeWall add_pad -signal sig60 -type sig -location {origin {x 3350.5 y 55}} -bump {row 22 col 21}
ICeWall add_pad -signal sig61 -type sig -location {origin {x 3453.5 y 55}} -bump {row 24 col 22}
ICeWall add_pad -signal sig62 -type sig -location {origin {x 3401.5 y 55}} -bump {row 23 col 22}
ICeWall add_pad -signal sig63 -type sig -location {origin {x 3608.5 y 55}} -bump {row 24 col 23}
ICeWall add_pad -signal sig64 -type sig -location {origin {x 3945 y 198.5}} -bump {row 24 col 24}
ICeWall add_pad -signal sig65 -type sig -location {origin {x 3945 y 353.5}} -bump {row 23 col 24}
ICeWall add_pad -signal sig66 -type sig -location {origin {x 3945 y 301.5}} -bump {row 23 col 23}
ICeWall add_pad -signal sig67 -type sig -location {origin {x 3945 y 508.5}} -bump {row 22 col 24}
ICeWall add_pad -signal sig68 -type sig -location {origin {x 3945 y 456.5}} -bump {row 22 col 23}
ICeWall add_pad -signal sig69 -type sig -location {origin {x 3945 y 560.5}} -bump {row 22 col 22}
ICeWall add_pad -signal sig70 -type sig -location {origin {x 3945 y 663.5}} -bump {row 21 col 24}
ICeWall add_pad -signal sig71 -type sig -location {origin {x 3945 y 611.5}} -bump {row 21 col 23}
ICeWall add_pad -signal sig72 -type sig -location {origin {x 3945 y 715.5}} -bump {row 21 col 22}
ICeWall add_pad -signal sig73 -type sig -location {origin {x 3945 y 818.5}} -bump {row 20 col 24}
ICeWall add_pad -signal sig74 -type sig -location {origin {x 3945 y 766.5}} -bump {row 20 col 23}
ICeWall add_pad -signal sig75 -type sig -location {origin {x 3945 y 870.5}} -bump {row 20 col 22}
ICeWall add_pad -signal sig76 -type sig -location {origin {x 3945 y 973.5}} -bump {row 19 col 24}
ICeWall add_pad -signal sig77 -type sig -location {origin {x 3945 y 921.5}} -bump {row 19 col 23}
ICeWall add_pad -signal sig78 -type sig -location {origin {x 3945 y 1025.5}} -bump {row 19 col 22}
ICeWall add_pad -signal sig79 -type sig -location {origin {x 3945 y 1128.5}} -bump {row 18 col 24}
ICeWall add_pad -signal sig80 -type sig -location {origin {x 3945 y 1076.5}} -bump {row 18 col 23}
ICeWall add_pad -signal sig81 -type sig -location {origin {x 3945 y 1180.5}} -bump {row 18 col 22}
ICeWall add_pad -signal sig82 -type sig -location {origin {x 3945 y 1283.5}} -bump {row 17 col 24}
ICeWall add_pad -signal sig83 -type sig -location {origin {x 3945 y 1231.5}} -bump {row 17 col 23}
ICeWall add_pad -signal sig84 -type sig -location {origin {x 3945 y 1335.5}} -bump {row 17 col 22}
ICeWall add_pad -signal sig85 -type sig -location {origin {x 3945 y 1438.5}} -bump {row 16 col 24}
ICeWall add_pad -signal sig86 -type sig -location {origin {x 3945 y 1386.5}} -bump {row 16 col 23}
ICeWall add_pad -signal sig87 -type sig -location {origin {x 3945 y 1490.5}} -bump {row 16 col 22}
ICeWall add_pad -signal sig88 -type sig -location {origin {x 3945 y 1593.5}} -bump {row 15 col 24}
ICeWall add_pad -signal sig89 -type sig -location {origin {x 3945 y 1541.5}} -bump {row 15 col 23}
ICeWall add_pad -signal sig90 -type sig -location {origin {x 3945 y 1645.5}} -bump {row 15 col 22}
ICeWall add_pad -signal sig91 -type sig -location {origin {x 3945 y 1748.5}} -bump {row 14 col 24}
ICeWall add_pad -signal sig92 -type sig -location {origin {x 3945 y 1696.5}} -bump {row 14 col 23}
ICeWall add_pad -signal sig93 -type sig -location {origin {x 3945 y 1800.5}} -bump {row 14 col 22}
ICeWall add_pad -signal sig94 -type sig -location {origin {x 3945 y 1903.5}} -bump {row 13 col 24}
ICeWall add_pad -signal sig95 -type sig -location {origin {x 3945 y 1851.5}} -bump {row 13 col 23}
ICeWall add_pad -signal sig96 -type sig -location {origin {x 3945 y 1955.5}} -bump {row 13 col 22}
ICeWall add_pad -signal sig97 -type sig -location {origin {x 3945 y 2058.5}} -bump {row 12 col 24}
ICeWall add_pad -signal sig98 -type sig -location {origin {x 3945 y 2006.5}} -bump {row 12 col 23}
ICeWall add_pad -signal sig99 -type sig -location {origin {x 3945 y 2110.5}} -bump {row 12 col 22}
ICeWall add_pad -signal sig100 -type sig -location {origin {x 3945 y 2213.5}} -bump {row 11 col 24}
ICeWall add_pad -signal sig101 -type sig -location {origin {x 3945 y 2161.5}} -bump {row 11 col 23}
ICeWall add_pad -signal sig102 -type sig -location {origin {x 3945 y 2265.5}} -bump {row 11 col 22}
ICeWall add_pad -signal sig103 -type sig -location {origin {x 3945 y 2368.5}} -bump {row 10 col 24}
ICeWall add_pad -signal sig104 -type sig -location {origin {x 3945 y 2316.5}} -bump {row 10 col 23}
ICeWall add_pad -signal sig105 -type sig -location {origin {x 3945 y 2420.5}} -bump {row 10 col 22}
ICeWall add_pad -signal sig106 -type sig -location {origin {x 3945 y 2523.5}} -bump {row 9 col 24}
ICeWall add_pad -signal sig107 -type sig -location {origin {x 3945 y 2471.5}} -bump {row 9 col 23}
ICeWall add_pad -signal sig108 -type sig -location {origin {x 3945 y 2575.5}} -bump {row 9 col 22}
ICeWall add_pad -signal sig109 -type sig -location {origin {x 3945 y 2678.5}} -bump {row 8 col 24}
ICeWall add_pad -signal sig110 -type sig -location {origin {x 3945 y 2626.5}} -bump {row 8 col 23}
ICeWall add_pad -signal sig111 -type sig -location {origin {x 3945 y 2730.5}} -bump {row 8 col 22}
ICeWall add_pad -signal sig112 -type sig -location {origin {x 3945 y 2833.5}} -bump {row 7 col 24}
ICeWall add_pad -signal sig113 -type sig -location {origin {x 3945 y 2781.5}} -bump {row 7 col 23}
ICeWall add_pad -signal sig114 -type sig -location {origin {x 3945 y 2885.5}} -bump {row 7 col 22}
ICeWall add_pad -signal sig115 -type sig -location {origin {x 3945 y 2988.5}} -bump {row 6 col 24}
ICeWall add_pad -signal sig116 -type sig -location {origin {x 3945 y 2936.5}} -bump {row 6 col 23}
ICeWall add_pad -signal sig117 -type sig -location {origin {x 3945 y 3040.5}} -bump {row 6 col 22}
ICeWall add_pad -signal sig118 -type sig -location {origin {x 3945 y 3143.5}} -bump {row 5 col 24}
ICeWall add_pad -signal sig119 -type sig -location {origin {x 3945 y 3091.5}} -bump {row 5 col 23}
ICeWall add_pad -signal sig120 -type sig -location {origin {x 3945 y 3195.5}} -bump {row 5 col 22}
ICeWall add_pad -signal sig121 -type sig -location {origin {x 3945 y 3298.5}} -bump {row 4 col 24}
ICeWall add_pad -signal sig122 -type sig -location {origin {x 3945 y 3246.5}} -bump {row 4 col 23}
ICeWall add_pad -signal sig123 -type sig -location {origin {x 3945 y 3350.5}} -bump {row 4 col 22}
ICeWall add_pad -signal sig124 -type sig -location {origin {x 3945 y 3453.5}} -bump {row 3 col 24}
ICeWall add_pad -signal sig125 -type sig -location {origin {x 3945 y 3401.5}} -bump {row 3 col 23}
ICeWall add_pad -signal sig126 -type sig -location {origin {x 3945 y 3608.5}} -bump {row 2 col 24}
ICeWall add_pad -signal sig127 -type sig -location {origin {x 3801.5 y 3945}} -bump {row 1 col 24}
ICeWall add_pad -signal sig128 -type sig -location {origin {x 3698.5 y 3945}} -bump {row 2 col 23}
ICeWall add_pad -signal sig129 -type sig -location {origin {x 3646.5 y 3945}} -bump {row 1 col 23}
ICeWall add_pad -signal sig130 -type sig -location {origin {x 3543.5 y 3945}} -bump {row 2 col 22}
ICeWall add_pad -signal sig131 -type sig -location {origin {x 3491.5 y 3945}} -bump {row 1 col 22}
ICeWall add_pad -signal sig132 -type sig -location {origin {x 3439.5 y 3945}} -bump {row 3 col 22}
ICeWall add_pad -signal sig133 -type sig -location {origin {x 3388.5 y 3945}} -bump {row 2 col 21}
ICeWall add_pad -signal sig134 -type sig -location {origin {x 3336.5 y 3945}} -bump {row 1 col 21}
ICeWall add_pad -signal sig135 -type sig -location {origin {x 3284.5 y 3945}} -bump {row 3 col 21}
ICeWall add_pad -signal sig136 -type sig -location {origin {x 3233.5 y 3945}} -bump {row 2 col 20}
ICeWall add_pad -signal sig137 -type sig -location {origin {x 3181.5 y 3945}} -bump {row 1 col 20}
ICeWall add_pad -signal sig138 -type sig -location {origin {x 3129.5 y 3945}} -bump {row 3 col 20}
ICeWall add_pad -signal sig139 -type sig -location {origin {x 3078.5 y 3945}} -bump {row 2 col 19}
ICeWall add_pad -signal sig140 -type sig -location {origin {x 3026.5 y 3945}} -bump {row 1 col 19}
ICeWall add_pad -signal sig141 -type sig -location {origin {x 2974.5 y 3945}} -bump {row 3 col 19}
ICeWall add_pad -signal sig142 -type sig -location {origin {x 2923.5 y 3945}} -bump {row 2 col 18}
ICeWall add_pad -signal sig143 -type sig -location {origin {x 2871.5 y 3945}} -bump {row 1 col 18}
ICeWall add_pad -signal sig144 -type sig -location {origin {x 2819.5 y 3945}} -bump {row 3 col 18}
ICeWall add_pad -signal sig145 -type sig -location {origin {x 2768.5 y 3945}} -bump {row 2 col 17}
ICeWall add_pad -signal sig146 -type sig -location {origin {x 2716.5 y 3945}} -bump {row 1 col 17}
ICeWall add_pad -signal sig147 -type sig -location {origin {x 2664.5 y 3945}} -bump {row 3 col 17}
ICeWall add_pad -signal sig148 -type sig -location {origin {x 2613.5 y 3945}} -bump {row 2 col 16}
ICeWall add_pad -signal sig149 -type sig -location {origin {x 2561.5 y 3945}} -bump {row 1 col 16}
ICeWall add_pad -signal sig150 -type sig -location {origin {x 2509.5 y 3945}} -bump {row 3 col 16}
ICeWall add_pad -signal sig151 -type sig -location {origin {x 2458.5 y 3945}} -bump {row 2 col 15}
ICeWall add_pad -signal sig152 -type sig -location {origin {x 2406.5 y 3945}} -bump {row 1 col 15}
ICeWall add_pad -signal sig153 -type sig -location {origin {x 2354.5 y 3945}} -bump {row 3 col 15}
ICeWall add_pad -signal sig154 -type sig -location {origin {x 2303.5 y 3945}} -bump {row 2 col 14}
ICeWall add_pad -signal sig155 -type sig -location {origin {x 2251.5 y 3945}} -bump {row 1 col 14}
ICeWall add_pad -signal sig156 -type sig -location {origin {x 2199.5 y 3945}} -bump {row 3 col 14}
ICeWall add_pad -signal sig157 -type sig -location {origin {x 2148.5 y 3945}} -bump {row 2 col 13}
ICeWall add_pad -signal sig158 -type sig -location {origin {x 2096.5 y 3945}} -bump {row 1 col 13}
ICeWall add_pad -signal sig159 -type sig -location {origin {x 2044.5 y 3945}} -bump {row 3 col 13}
ICeWall add_pad -signal sig160 -type sig -location {origin {x 1993.5 y 3945}} -bump {row 2 col 12}
ICeWall add_pad -signal sig161 -type sig -location {origin {x 1941.5 y 3945}} -bump {row 1 col 12}
ICeWall add_pad -signal sig162 -type sig -location {origin {x 1889.5 y 3945}} -bump {row 3 col 12}
ICeWall add_pad -signal sig163 -type sig -location {origin {x 1838.5 y 3945}} -bump {row 2 col 11}
ICeWall add_pad -signal sig164 -type sig -location {origin {x 1786.5 y 3945}} -bump {row 1 col 11}
ICeWall add_pad -signal sig165 -type sig -location {origin {x 1734.5 y 3945}} -bump {row 3 col 11}
ICeWall add_pad -signal sig166 -type sig -location {origin {x 1683.5 y 3945}} -bump {row 2 col 10}
ICeWall add_pad -signal sig167 -type sig -location {origin {x 1631.5 y 3945}} -bump {row 1 col 10}
ICeWall add_pad -signal sig168 -type sig -location {origin {x 1579.5 y 3945}} -bump {row 3 col 10}
ICeWall add_pad -signal sig169 -type sig -location {origin {x 1528.5 y 3945}} -bump {row 2 col 9}
ICeWall add_pad -signal sig170 -type sig -location {origin {x 1476.5 y 3945}} -bump {row 1 col 9}
ICeWall add_pad -signal sig171 -type sig -location {origin {x 1424.5 y 3945}} -bump {row 3 col 9}
ICeWall add_pad -signal sig172 -type sig -location {origin {x 1373.5 y 3945}} -bump {row 2 col 8}
ICeWall add_pad -signal sig173 -type sig -location {origin {x 1321.5 y 3945}} -bump {row 1 col 8}
ICeWall add_pad -signal sig174 -type sig -location {origin {x 1269.5 y 3945}} -bump {row 3 col 8}
ICeWall add_pad -signal sig175 -type sig -location {origin {x 1218.5 y 3945}} -bump {row 2 col 7}
ICeWall add_pad -signal sig176 -type sig -location {origin {x 1166.5 y 3945}} -bump {row 1 col 7}
ICeWall add_pad -signal sig177 -type sig -location {origin {x 1114.5 y 3945}} -bump {row 3 col 7}
ICeWall add_pad -signal sig178 -type sig -location {origin {x 1063.5 y 3945}} -bump {row 2 col 6}
ICeWall add_pad -signal sig179 -type sig -location {origin {x 1011.5 y 3945}} -bump {row 1 col 6}
ICeWall add_pad -signal sig180 -type sig -location {origin {x 959.5 y 3945}} -bump {row 3 col 6}
ICeWall add_pad -signal sig181 -type sig -location {origin {x 908.5 y 3945}} -bump {row 2 col 5}
ICeWall add_pad -signal sig182 -type sig -location {origin {x 856.5 y 3945}} -bump {row 1 col 5}
ICeWall add_pad -signal sig183 -type sig -location {origin {x 804.5 y 3945}} -bump {row 3 col 5}
ICeWall add_pad -signal sig184 -type sig -location {origin {x 753.5 y 3945}} -bump {row 2 col 4}
ICeWall add_pad -signal sig185 -type sig -location {origin {x 701.5 y 3945}} -bump {row 1 col 4}
ICeWall add_pad -signal sig186 -type sig -location {origin {x 649.5 y 3945}} -bump {row 3 col 4}
ICeWall add_pad -signal sig187 -type sig -location {origin {x 598.5 y 3945}} -bump {row 2 col 3}
ICeWall add_pad -signal sig188 -type sig -location {origin {x 546.5 y 3945}} -bump {row 1 col 3}
ICeWall add_pad -signal sig189 -type sig -location {origin {x 391.5 y 3945}} -bump {row 1 col 2}
ICeWall add_pad -signal sig190 -type sig -location {origin {x 55 y 3801.5}} -bump {row 1 col 1}
ICeWall add_pad -signal sig191 -type sig -location {origin {x 55 y 3698.5}} -bump {row 2 col 2}
ICeWall add_pad -signal sig192 -type sig -location {origin {x 55 y 3646.5}} -bump {row 2 col 1}
ICeWall add_pad -signal sig193 -type sig -location {origin {x 55 y 3543.5}} -bump {row 3 col 2}
ICeWall add_pad -signal sig194 -type sig -location {origin {x 55 y 3491.5}} -bump {row 3 col 1}
ICeWall add_pad -signal sig195 -type sig -location {origin {x 55 y 3439.5}} -bump {row 3 col 3}
ICeWall add_pad -signal sig196 -type sig -location {origin {x 55 y 3388.5}} -bump {row 4 col 2}
ICeWall add_pad -signal sig197 -type sig -location {origin {x 55 y 3336.5}} -bump {row 4 col 1}
ICeWall add_pad -signal sig198 -type sig -location {origin {x 55 y 3284.5}} -bump {row 4 col 3}
ICeWall add_pad -signal sig199 -type sig -location {origin {x 55 y 3233.5}} -bump {row 5 col 2}
ICeWall add_pad -signal sig200 -type sig -location {origin {x 55 y 3181.5}} -bump {row 5 col 1}
ICeWall add_pad -signal sig201 -type sig -location {origin {x 55 y 3129.5}} -bump {row 5 col 3}
ICeWall add_pad -signal sig202 -type sig -location {origin {x 55 y 3078.5}} -bump {row 6 col 2}
ICeWall add_pad -signal sig203 -type sig -location {origin {x 55 y 3026.5}} -bump {row 6 col 1}
ICeWall add_pad -signal sig204 -type sig -location {origin {x 55 y 2974.5}} -bump {row 6 col 3}
ICeWall add_pad -signal sig205 -type sig -location {origin {x 55 y 2923.5}} -bump {row 7 col 2}
ICeWall add_pad -signal sig206 -type sig -location {origin {x 55 y 2871.5}} -bump {row 7 col 1}
ICeWall add_pad -signal sig207 -type sig -location {origin {x 55 y 2819.5}} -bump {row 7 col 3}
ICeWall add_pad -signal sig208 -type sig -location {origin {x 55 y 2768.5}} -bump {row 8 col 2}
ICeWall add_pad -signal sig209 -type sig -location {origin {x 55 y 2716.5}} -bump {row 8 col 1}
ICeWall add_pad -signal sig210 -type sig -location {origin {x 55 y 2664.5}} -bump {row 8 col 3}
ICeWall add_pad -signal sig211 -type sig -location {origin {x 55 y 2613.5}} -bump {row 9 col 2}
ICeWall add_pad -signal sig212 -type sig -location {origin {x 55 y 2561.5}} -bump {row 9 col 1}
ICeWall add_pad -signal sig213 -type sig -location {origin {x 55 y 2509.5}} -bump {row 9 col 3}
ICeWall add_pad -signal sig214 -type sig -location {origin {x 55 y 2458.5}} -bump {row 10 col 2}
ICeWall add_pad -signal sig215 -type sig -location {origin {x 55 y 2406.5}} -bump {row 10 col 1}
ICeWall add_pad -signal sig216 -type sig -location {origin {x 55 y 2354.5}} -bump {row 10 col 3}
ICeWall add_pad -signal sig217 -type sig -location {origin {x 55 y 2303.5}} -bump {row 11 col 2}
ICeWall add_pad -signal sig218 -type sig -location {origin {x 55 y 2251.5}} -bump {row 11 col 1}
ICeWall add_pad -signal sig219 -type sig -location {origin {x 55 y 2199.5}} -bump {row 11 col 3}
ICeWall add_pad -signal sig220 -type sig -location {origin {x 55 y 2148.5}} -bump {row 12 col 2}
ICeWall add_pad -signal sig221 -type sig -location {origin {x 55 y 2096.5}} -bump {row 12 col 1}
ICeWall add_pad -signal sig222 -type sig -location {origin {x 55 y 2044.5}} -bump {row 12 col 3}
ICeWall add_pad -signal sig223 -type sig -location {origin {x 55 y 1993.5}} -bump {row 13 col 2}
ICeWall add_pad -signal sig224 -type sig -location {origin {x 55 y 1941.5}} -bump {row 13 col 1}
ICeWall add_pad -signal sig225 -type sig -location {origin {x 55 y 1889.5}} -bump {row 13 col 3}
ICeWall add_pad -signal sig226 -type sig -location {origin {x 55 y 1838.5}} -bump {row 14 col 2}
ICeWall add_pad -signal sig227 -type sig -location {origin {x 55 y 1786.5}} -bump {row 14 col 1}
ICeWall add_pad -signal sig228 -type sig -location {origin {x 55 y 1734.5}} -bump {row 14 col 3}
ICeWall add_pad -signal sig229 -type sig -location {origin {x 55 y 1683.5}} -bump {row 15 col 2}
ICeWall add_pad -signal sig230 -type sig -location {origin {x 55 y 1631.5}} -bump {row 15 col 1}
ICeWall add_pad -signal sig231 -type sig -location {origin {x 55 y 1579.5}} -bump {row 15 col 3}
ICeWall add_pad -signal sig232 -type sig -location {origin {x 55 y 1528.5}} -bump {row 16 col 2}
ICeWall add_pad -signal sig233 -type sig -location {origin {x 55 y 1476.5}} -bump {row 16 col 1}
ICeWall add_pad -signal sig234 -type sig -location {origin {x 55 y 1424.5}} -bump {row 16 col 3}
ICeWall add_pad -signal sig235 -type sig -location {origin {x 55 y 1373.5}} -bump {row 17 col 2}
ICeWall add_pad -signal sig236 -type sig -location {origin {x 55 y 1321.5}} -bump {row 17 col 1}
ICeWall add_pad -signal sig237 -type sig -location {origin {x 55 y 1269.5}} -bump {row 17 col 3}
ICeWall add_pad -signal sig238 -type sig -location {origin {x 55 y 1218.5}} -bump {row 18 col 2}
ICeWall add_pad -signal sig239 -type sig -location {origin {x 55 y 1166.5}} -bump {row 18 col 1}
ICeWall add_pad -signal sig240 -type sig -location {origin {x 55 y 1114.5}} -bump {row 18 col 3}
ICeWall add_pad -signal sig241 -type sig -location {origin {x 55 y 1063.5}} -bump {row 19 col 2}
ICeWall add_pad -signal sig242 -type sig -location {origin {x 55 y 1011.5}} -bump {row 19 col 1}
ICeWall add_pad -signal sig243 -type sig -location {origin {x 55 y 959.5}} -bump {row 19 col 3}
ICeWall add_pad -signal sig244 -type sig -location {origin {x 55 y 908.5}} -bump {row 20 col 2}
ICeWall add_pad -signal sig245 -type sig -location {origin {x 55 y 856.5}} -bump {row 20 col 1}
ICeWall add_pad -signal sig246 -type sig -location {origin {x 55 y 804.5}} -bump {row 20 col 3}
ICeWall add_pad -signal sig247 -type sig -location {origin {x 55 y 753.5}} -bump {row 21 col 2}
ICeWall add_pad -signal sig248 -type sig -location {origin {x 55 y 701.5}} -bump {row 21 col 1}
ICeWall add_pad -signal sig249 -type sig -location {origin {x 55 y 649.5}} -bump {row 21 col 3}
ICeWall add_pad -signal sig251 -type sig -location {origin {x 55 y 546.5}} -bump {row 22 col 1}
ICeWall add_pad -signal sig252 -type sig -location {origin {x 55 y 391.5}} -bump {row 23 col 1}

if {[catch {ICeWall init_footprint} msg]} {
  puts $errorInfo
  puts $msg
  return
}

set def_file [make_result_file "flipchip_test.api.rdl.def"]
set def1_file [make_result_file "flipchip_test.api.def"]

write_def $def_file
exec sed -e "/END SPECIALNETS/r[ICeWall::get_footprint_rdl_cover_file_name]" $def_file > $def1_file

diff_files $def1_file "flipchip_test.defok"
