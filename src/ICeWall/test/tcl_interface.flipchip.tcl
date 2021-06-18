source "helpers.tcl"

read_lef NangateOpenCellLibrary.mod.lef
read_lef dummy_pads.lef

read_liberty dummy_pads.lib

read_verilog soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.flipchip.v

link_design soc_bsg_black_parrot

initialize_floorplan \
  -die_area  {0 0 3000.000 3000.000} \
  -core_area {180.012 180.096 2819.964 2819.712} \
  -site      FreePDK45_38x28_10R_NP_162NW_34O
make_tracks

# Load library defintions
define_pad_cell \
  -name PAD \
  -orient {bottom R0 right R90 top R180 left R270} \
  -type bondpad \
  -pad_pin_name PAD 

define_pad_cell \
  -name PADCELL_SIG \
  -type sig \
  -cell_name {top PADCELL_SIG_V bottom PADCELL_SIG_V left PADCELL_SIG_H right PADCELL_SIG_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name PAD
  
define_pad_cell \
  -name PADCELL_VDD \
  -type vdd \
  -cell_name {top PADCELL_VDD_V bottom PADCELL_VDD_V left PADCELL_VDD_H right PADCELL_VDD_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name VDD

define_pad_cell \
  -name PADCELL_VSS \
  -type vss \
  -cell_name {top PADCELL_VSS_V bottom PADCELL_VSS_V left PADCELL_VSS_H right PADCELL_VSS_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name VSS

define_pad_cell \
  -name PADCELL_VDDIO \
  -type vddio \
  -cell_name {top PADCELL_VDDIO_V bottom PADCELL_VDDIO_V left PADCELL_VDDIO_H right PADCELL_VDDIO_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name DVDD

define_pad_cell \
  -name PADCELL_VSSIO \
  -type vssio \
  -cell_name {top PADCELL_VSSIO_V bottom PADCELL_VSSIO_V left PADCELL_VSSIO_H right PADCELL_VSSIO_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name DVSS

define_pad_cell \
  -name PADCELL_CBRK \
  -type cbk \
  -cell_name {bottom PADCELL_CBRK_V right PADCELL_CBRK_H top PADCELL_CBRK_V left PADCELL_CBRK_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -break_signals {RETN {RETNA RETNB} SNS {SNSA SNSB}} \
  -physical_only 
  
define_pad_cell \
  -name PADCELL_PWRDET \
  -type pdt \
  -cell_name {bottom PADCELL_PWRDET_V right PADCELL_PWRDET_H top PADCELL_PWRDET_V left PADCELL_PWRDET_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -physical_only 

define_pad_cell \
  -name PADCELL_FBRK \
  -type fbk \
  -cell_name {bottom PADCELL_FBRK_V right PADCELL_FBRK_H top PADCELL_FBRK_V left PADCELL_FBRK_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -break_signals {RETN {RETNA RETNB} SNS {SNSA SNSB} DVDD {DVDDA DVDDB} DVSS {DVSSA DVSSB}} \
  -physical_only 

define_pad_cell \
  -name PAD_FILL5 \
  -type fill \
  -cell_name {bottom PAD_FILL5_V right PAD_FILL5_H top PAD_FILL5_V left PAD_FILL5_H} \
  -orient {bottom R0 right MY top R180 left MX} \
  -physical_only 
  
define_pad_cell \
  -name PAD_FILL1 \
  -type fill \
  -cell_name {bottom PAD_FILL1_V right PAD_FILL1_H top PAD_FILL1_V left PAD_FILL1_H} \
  -orient {bottom R0 right MY top R180 left MX} \
  -physical_only 
  
define_pad_cell \
  -name PAD_CORNER \
  -type corner \
  -orient {ll R0 lr R90 ur R180 ul R270} \
  -physical_only 
  
define_pad_cell \
  -name DUMMY_BUMP \
  -cell_name DUMMY_BUMP \
  -type bump \
  -physical_only 
  
set_bump_options \
  -pitch 160 \
  -bump_pin_name PAD \
  -spacing_to_edge 165 \
  -cell_name DUMMY_BUMP \
  -num_pads_per_tile 5 \
  -rdl_layer metal10 \
  -rdl_width 10 \
  -rdl_spacing 10

set_padring_options \
  -type flipchip \
  -power  {VDD DVDD_0 DVDD_1} \
  -ground {VSS DVSS_0 DVSS_1} \
  -offsets 35 \
  -pin_layer metal10 \
  -pad_inst_name "%s" \
  -pad_pin_name "PAD" \
  -connect_by_abutment {SNS RETN DVDD DVSS}

place_cell -cell MARKER -inst_name u_marker_0 -origin {1197.5 1199.3} -orient R0 -status FIRM

puts "Trigger errors"
# Trigger errors
catch {add_pad -edge other  -signal p_ddr_dm_1_o         -type sig   -location {centre {x  292.000 y  105.000}} -bondpad {centre {x  292.000 y   63.293}}}
puts "No more errors expected"

# Define the same padring for soc_bsg_black_parrot_nangate45 using TCL commands, rather than strategy file.
# 
add_pad -edge bottom -inst_name u_cbrk0                 -type cbk   -location {origin {x  140 y   35}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o               -type sig   -location {origin {x  205 y   35}} -bump {row 23 col 1}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o               -type sig   -location {origin {x  205 y   35}} -bump {row 17 col 0}}
add_pad -edge bottom -signal p_ddr_dm_1_o               -type sig   -location {origin {x  205 y   35}} -bump {row 17 col 1}
add_pad -edge bottom -signal p_ddr_dqs_n_1_io           -type sig   -location {origin {x  335 y   35}} -bump {row 16 col 2}
add_pad -edge bottom -inst_name u_vzz_0 -signal DVSS_0  -type vssio -location {origin {x  365 y   35}} -bump {row 17 col 2}
add_pad -edge bottom -inst_name u_v18_0 -signal DVDD_0  -type vddio -location {origin {x  495 y   35}} -bump {row 16 col 3}
add_pad -edge bottom -signal p_ddr_dqs_p_1_io           -type sig   -location {origin {x  525 y   35}} -bump {row 17 col 3}
add_pad -edge bottom -inst_name u_vdd_0 -signal VDD     -type vdd   -location {origin {x  555 y   35}} -bump {row 15 col 3}
add_pad -edge bottom -inst_name u_vss_0 -signal VSS     -type vss   -location {origin {x  625 y   35}} -bump {row 14 col 4}
add_pad -edge bottom -signal p_ddr_ba_2_o               -type sig   -location {origin {x  655 y   35}} -bump {row 16 col 4}
add_pad -edge bottom -inst_name u_pwrdet0               -type pdt   -location {origin {x  685 y   35}}
add_pad -edge bottom -signal p_ddr_ba_1_o               -type sig   -location {origin {x  690 y   35}} -bump {row 17 col 4}
add_pad -edge bottom -signal p_ddr_ba_0_o               -type sig   -location {origin {x  720 y   35}} -bump {row 15 col 4}
add_pad -edge bottom -inst_name u_vzz_1 -signal DVSS_0  -type vssio -location {origin {x  785 y   35}} -bump {row 14 col 5}
add_pad -edge bottom -inst_name u_v18_1 -signal DVDD_0  -type vddio -location {origin {x  815 y   35}} -bump {row 16 col 5}
add_pad -edge bottom -signal p_ddr_addr_15_o            -type sig   -location {origin {x  845 y   35}} -bump {row 17 col 5}
add_pad -edge bottom -signal p_ddr_addr_14_o            -type sig   -location {origin {x  875 y   35}} -bump {row 15 col 5}
add_pad -edge bottom -inst_name u_cbrk1                 -type cbk   -location {origin {x  905 y   35}}
add_pad -edge bottom -signal p_ddr_addr_13_o            -type sig   -location {origin {x  910 y   35}} -bump {row 13 col 5}
add_pad -edge bottom -signal p_ddr_addr_12_o            -type sig   -location {origin {x  945 y   35}} -bump {row 14 col 6}
add_pad -edge bottom -inst_name u_vzz_2 -signal DVSS_0  -type vssio -location {origin {x  975 y   35}} -bump {row 16 col 6}
add_pad -edge bottom -inst_name u_v18_2 -signal DVDD_0  -type vddio -location {origin {x 1005 y   35}} -bump {row 17 col 6}
add_pad -edge bottom -signal p_ddr_addr_11_o            -type sig   -location {origin {x 1035 y   35}} -bump {row 15 col 6}
add_pad -edge bottom -signal p_ddr_addr_10_o            -type sig   -location {origin {x 1065 y   35}} -bump {row 13 col 6}
add_pad -edge bottom -inst_name u_vdd_1 -signal VDD     -type vdd   -location {origin {x 1105 y   35}} -bump {row 14 col 7}
add_pad -edge bottom -inst_name u_vss_1 -signal VSS     -type vss   -location {origin {x 1135 y   35}} -bump {row 16 col 7}
add_pad -edge bottom -inst_name u_pwrdet1               -type pdt   -location {origin {x 1165 y   35}}
add_pad -edge bottom -signal p_ddr_addr_9_o             -type sig   -location {origin {x 1170 y   35}} -bump {row 17 col 7}
add_pad -edge bottom -signal p_ddr_addr_8_o             -type sig   -location {origin {x 1200 y   35}} -bump {row 15 col 7}
add_pad -edge bottom -inst_name u_vzz_3 -signal DVSS_0  -type vssio -location {origin {x 1230 y   35}} -bump {row 13 col 7}
add_pad -edge bottom -inst_name u_v18_3 -signal DVDD_0  -type vddio -location {origin {x 1265 y   35}} -bump {row 14 col 8}
add_pad -edge bottom -signal p_ddr_addr_7_o             -type sig   -location {origin {x 1295 y   35}} -bump {row 16 col 8}
add_pad -edge bottom -signal p_ddr_addr_6_o             -type sig   -location {origin {x 1325 y   35}} -bump {row 17 col 8}
add_pad -edge bottom -inst_name u_cbrk2                 -type cbk   -location {origin {x 1355 y   35}}
add_pad -edge bottom -signal p_ddr_addr_5_o             -type sig   -location {origin {x 1360 y   35}} -bump {row 15 col 8}
add_pad -edge bottom -signal p_ddr_addr_4_o             -type sig   -location {origin {x 1390 y   35}} -bump {row 13 col 8}
add_pad -edge bottom -inst_name u_vzz_4 -signal DVSS_0  -type vssio -location {origin {x 1425 y   35}} -bump {row 14 col 9}
add_pad -edge bottom -inst_name u_v18_4 -signal DVDD_0  -type vddio -location {origin {x 1455 y   35}} -bump {row 16 col 9}
add_pad -edge bottom -signal p_ddr_addr_3_o             -type sig   -location {origin {x 1485 y   35}} -bump {row 17 col 9}
add_pad -edge bottom -signal p_ddr_addr_2_o             -type sig   -location {origin {x 1515 y   35}} -bump {row 15 col 9}
add_pad -edge bottom -inst_name u_pwrdet2               -type pdt   -location {origin {x 1545 y   35}}
add_pad -edge bottom -signal p_ddr_addr_1_o             -type sig   -location {origin {x 1550 y   35}} -bump {row 13 col 9}
add_pad -edge bottom -inst_name u_vdd_2 -signal VDD     -type vdd   -location {origin {x 1585 y   35}} -bump {row 14 col 10}
add_pad -edge bottom -inst_name u_vss_2 -signal VSS     -type vss   -location {origin {x 1615 y   35}} -bump {row 16 col 10}
add_pad -edge bottom -signal p_ddr_addr_0_o             -type sig   -location {origin {x 1645 y   35}} -bump {row 17 col 10}
add_pad -edge bottom -inst_name u_vzz_5 -signal DVSS_0  -type vssio -location {origin {x 1675 y   35}} -bump {row 15 col 10}
add_pad -edge bottom -inst_name u_v18_5 -signal DVDD_0  -type vddio -location {origin {x 1705 y   35}} -bump {row 13 col 10}
add_pad -edge bottom -signal p_ddr_odt_o                -type sig   -location {origin {x 1745 y   35}} -bump {row 14 col 11}
add_pad -edge bottom -signal p_ddr_reset_n_o            -type sig   -location {origin {x 1775 y   35}} -bump {row 16 col 11}
add_pad -edge bottom -inst_name u_cbrk3                 -type cbk   -location {origin {x 1805 y   35}}
add_pad -edge bottom -signal p_ddr_we_n_o               -type sig   -location {origin {x 1810 y   35}} -bump {row 17 col 11}
add_pad -edge bottom -signal p_ddr_cas_n_o              -type sig   -location {origin {x 1840 y   35}} -bump {row 15 col 11}
add_pad -edge bottom -inst_name u_vzz_6 -signal DVSS_0  -type vssio -location {origin {x 1870 y   35}} -bump {row 13 col 11}
add_pad -edge bottom -inst_name u_v18_6 -signal DVDD_0  -type vddio -location {origin {x 1905 y   35}} -bump {row 14 col 12}
add_pad -edge bottom -signal p_ddr_ras_n_o              -type sig   -location {origin {x 1935 y   35}} -bump {row 16 col 12}
add_pad -edge bottom -signal p_ddr_cs_n_o               -type sig   -location {origin {x 1965 y   35}} -bump {row 17 col 12}
add_pad -edge bottom -inst_name u_pwrdet3               -type pdt   -location {origin {x 1995 y   35}}
add_pad -edge bottom -signal p_ddr_cke_o                -type sig   -location {origin {x 2000 y   35}} -bump {row 15 col 12}
add_pad -edge bottom -signal p_ddr_ck_n_o               -type sig   -location {origin {x 2030 y   35}} -bump {row 13 col 12}
add_pad -edge bottom -inst_name u_vzz_7 -signal DVSS_0  -type vssio -location {origin {x 2065 y   35}} -bump {row 14 col 13}
add_pad -edge bottom -inst_name u_v18_7 -signal DVDD_0  -type vddio -location {origin {x 2095 y   35}} -bump {row 16 col 13}
add_pad -edge bottom -inst_name u_vdd_3 -signal VDD     -type vdd   -location {origin {x 2125 y   35}} -bump {row 17 col 13}
add_pad -edge bottom -inst_name u_vss_3 -signal VSS     -type vss   -location {origin {x 2155 y   35}} -bump {row 15 col 13}
add_pad -edge bottom -signal p_ddr_ck_p_o               -type sig   -location {origin {x 2255 y   35}} -bump {row 16 col 14}
add_pad -edge bottom -signal p_ddr_dqs_n_2_io           -type sig   -location {origin {x 2285 y   35}} -bump {row 17 col 14}
add_pad -edge bottom -inst_name u_cbrk4                 -type cbk   -location {origin {x 2315 y   35}}
add_pad -edge bottom -signal p_ddr_dqs_p_2_io           -type sig   -location {origin {x 2320 y   35}} -bump {row 15 col 14}
add_pad -edge bottom -signal p_ddr_dm_2_o               -type sig   -location {origin {x 2415 y   35}} -bump {row 16 col 15}
add_pad -edge bottom -inst_name u_vzz_8 -signal DVSS_0  -type vssio -location {origin {x 2445 y   35}} -bump {row 17 col 15}
add_pad -edge bottom -inst_name u_v18_8 -signal DVDD_0  -type vddio -location {origin {x 2605 y   35}} -bump {row 17 col 16}
add_pad -edge right  -signal p_ddr_dq_23_io             -type sig   -location {origin {x 2965 y  205}} -bump {row 17 col 17}
add_pad -edge right  -signal p_ddr_dq_22_io             -type sig   -location {origin {x 2965 y  335}} -bump {row 16 col 16}
add_pad -edge right  -inst_name u_pwrdet4               -type pdt   -location {origin {x 2965 y  365}}
add_pad -edge right  -signal p_ddr_dq_21_io             -type sig   -location {origin {x 2965 y  370}} -bump {row 16 col 17}
add_pad -edge right  -inst_name u_vdd_4 -signal VDD     -type vdd   -location {origin {x 2965 y  495}} -bump {row 15 col 16}
add_pad -edge right  -inst_name u_vss_4 -signal VSS     -type vss   -location {origin {x 2965 y  525}} -bump {row 15 col 17}
add_pad -edge right  -signal p_ddr_dq_20_io             -type sig   -location {origin {x 2965 y  555}} -bump {row 15 col 15}
add_pad -edge right  -inst_name u_vzz_9 -signal DVSS_0  -type vssio -location {origin {x 2965 y  625}} -bump {row 14 col 14}
add_pad -edge right  -inst_name u_v18_9 -signal DVDD_0  -type vddio -location {origin {x 2965 y  655}} -bump {row 14 col 16}
add_pad -edge right  -signal p_ddr_dq_19_io             -type sig   -location {origin {x 2965 y  685}} -bump {row 14 col 17}
add_pad -edge right  -signal p_ddr_dq_18_io             -type sig   -location {origin {x 2965 y  715}} -bump {row 14 col 15}
add_pad -edge right  -inst_name u_cbrk5                 -type cbk   -location {origin {x 2965 y  780}}
add_pad -edge right  -signal p_ddr_dq_17_io             -type sig   -location {origin {x 2965 y  785}} -bump {row 13 col 14}
add_pad -edge right  -signal p_ddr_dq_16_io             -type sig   -location {origin {x 2965 y  815}} -bump {row 13 col 16}
add_pad -edge right  -inst_name u_vzz_10 -signal DVSS_0 -type vssio -location {origin {x 2965 y  845}} -bump {row 13 col 17}
add_pad -edge right  -inst_name u_v18_10 -signal DVDD_0 -type vddio -location {origin {x 2965 y  875}} -bump {row 13 col 15}
add_pad -edge right  -signal p_ddr_dq_31_io             -type sig   -location {origin {x 2965 y  905}} -bump {row 13 col 13}
add_pad -edge right  -signal p_ddr_dq_30_io             -type sig   -location {origin {x 2965 y  945}} -bump {row 12 col 14}
add_pad -edge right  -inst_name u_pwrdet5               -type pdt   -location {origin {x 2965 y  975}}
add_pad -edge right  -signal p_ddr_dq_29_io             -type sig   -location {origin {x 2965 y  980}} -bump {row 12 col 16}
add_pad -edge right  -signal p_ddr_dq_28_io             -type sig   -location {origin {x 2965 y 1010}} -bump {row 12 col 17}
add_pad -edge right  -inst_name u_vzz_11 -signal DVSS_0 -type vssio -location {origin {x 2965 y 1040}} -bump {row 12 col 15}
add_pad -edge right  -inst_name u_v18_11 -signal DVDD_0 -type vddio -location {origin {x 2965 y 1070}} -bump {row 12 col 13}
add_pad -edge right  -signal p_ddr_dq_27_io             -type sig   -location {origin {x 2965 y 1105}} -bump {row 11 col 14}
add_pad -edge right  -inst_name u_vdd_5 -signal VDD     -type vdd   -location {origin {x 2965 y 1135}} -bump {row 11 col 16}
add_pad -edge right  -inst_name u_vss_5 -signal VSS     -type vss   -location {origin {x 2965 y 1165}} -bump {row 11 col 17}
add_pad -edge right  -signal p_ddr_dq_26_io             -type sig   -location {origin {x 2965 y 1195}} -bump {row 11 col 15}
add_pad -edge right  -inst_name u_cbrk6                 -type cbk   -location {origin {x 2965 y 1225}}
add_pad -edge right  -signal p_ddr_dq_25_io             -type sig   -location {origin {x 2965 y 1230}} -bump {row 11 col 13}
add_pad -edge right  -signal p_ddr_dq_24_io             -type sig   -location {origin {x 2965 y 1265}} -bump {row 10 col 14}
add_pad -edge right  -inst_name u_vzz_12 -signal DVSS_0 -type vssio -location {origin {x 2965 y 1295}} -bump {row 10 col 16}
add_pad -edge right  -inst_name u_v18_12 -signal DVDD_0 -type vddio -location {origin {x 2965 y 1325}} -bump {row 10 col 17}
add_pad -edge right  -signal p_ddr_dqs_n_3_io           -type sig   -location {origin {x 2965 y 1355}} -bump {row 10 col 15}
add_pad -edge right  -signal p_ddr_dqs_p_3_io           -type sig   -location {origin {x 2965 y 1385}} -bump {row 10 col 13}
add_pad -edge right  -inst_name u_pwrdet6               -type pdt   -location {origin {x 2965 y 1420}}
add_pad -edge right  -signal p_ddr_dm_3_o               -type sig   -location {origin {x 2965 y 1425}} -bump {row 9 col 14}
add_pad -edge right  -signal p_bsg_tag_clk_i            -type sig   -location {origin {x 2965 y 1455}} -bump {row 9 col 16}
add_pad -edge right  -inst_name u_vzz_13 -signal DVSS_0 -type vssio -location {origin {x 2965 y 1485}} -bump {row 9 col 17}
add_pad -edge right  -inst_name u_v18_13 -signal DVDD_0 -type vddio -location {origin {x 2965 y 1515}} -bump {row 9 col 15}
add_pad -edge right  -signal p_bsg_tag_data_i           -type sig   -location {origin {x 2965 y 1545}} -bump {row 9 col 13}
add_pad -edge right  -signal p_bsg_tag_en_i             -type sig   -location {origin {x 2965 y 1585}} -bump {row 8 col 14}
add_pad -edge right  -inst_name u_cbrk7                 -type cbk   -location {origin {x 2965 y 1615}}
add_pad -edge right  -signal p_ci_8_i                   -type sig   -location {origin {x 2965 y 1620}} -bump {row 8 col 16}
add_pad -edge right  -inst_name u_vdd_6 -signal VDD     -type vdd   -location {origin {x 2965 y 1650}} -bump {row 8 col 17}
add_pad -edge right  -inst_name u_vss_6 -signal VSS     -type vss   -location {origin {x 2965 y 1680}} -bump {row 8 col 15}
add_pad -edge right  -signal p_ci_7_i                   -type sig   -location {origin {x 2965 y 1710}} -bump {row 8 col 13}
add_pad -edge right  -inst_name u_vzz_14 -signal DVSS_0 -type vssio -location {origin {x 2965 y 1745}} -bump {row 7 col 14}
add_pad -edge right  -inst_name u_v18_14 -signal DVDD_0 -type vddio -location {origin {x 2965 y 1775}} -bump {row 7 col 16}
add_pad -edge right  -signal p_ci_6_i                   -type sig   -location {origin {x 2965 y 1805}} -bump {row 7 col 17}
add_pad -edge right  -signal p_ci_5_i                   -type sig   -location {origin {x 2965 y 1835}} -bump {row 7 col 15}
add_pad -edge right  -inst_name u_pwrdet7               -type pdt   -location {origin {x 2965 y 1865}}
add_pad -edge right  -signal p_ci_v_i                   -type sig   -location {origin {x 2965 y 1870}} -bump {row 7 col 13}
add_pad -edge right  -signal p_ci_tkn_o                 -type sig   -location {origin {x 2965 y 1905}} -bump {row 6 col 14}
add_pad -edge right  -inst_name u_vzz_15 -signal DVSS_0 -type vssio -location {origin {x 2965 y 1935}} -bump {row 6 col 16}
add_pad -edge right  -inst_name u_v18_15 -signal DVDD_0 -type vddio -location {origin {x 2965 y 1965}} -bump {row 6 col 17}
add_pad -edge right  -signal p_ci_clk_i                 -type sig   -location {origin {x 2965 y 1995}} -bump {row 6 col 15}
add_pad -edge right  -signal p_ci_4_i                   -type sig   -location {origin {x 2965 y 2025}} -bump {row 6 col 13}
add_pad -edge right  -inst_name u_cbrk8                 -type cbk   -location {origin {x 2965 y 2060}}
add_pad -edge right  -signal p_ci_3_i                   -type sig   -location {origin {x 2965 y 2065}} -bump {row 5 col 14}
add_pad -edge right  -signal p_ci_2_i                   -type sig   -location {origin {x 2965 y 2095}} -bump {row 5 col 16}
add_pad -edge right  -inst_name u_vzz_16 -signal DVSS_0 -type vssio -location {origin {x 2965 y 2125}} -bump {row 5 col 17}
add_pad -edge right  -inst_name u_v18_16 -signal DVDD_0 -type vddio -location {origin {x 2965 y 2155}} -bump {row 5 col 15}
add_pad -edge right  -signal p_ci_1_i                   -type sig   -location {origin {x 2965 y 2255}} -bump {row 4 col 16}
add_pad -edge right  -inst_name u_vdd_7 -signal VDD     -type vdd   -location {origin {x 2965 y 2285}} -bump {row 4 col 17}
add_pad -edge right  -inst_name u_vss_7 -signal VSS     -type vss   -location {origin {x 2965 y 2315}} -bump {row 4 col 15}
add_pad -edge right  -signal p_ci_0_i                   -type sig   -location {origin {x 2965 y 2415}} -bump {row 3 col 16}
add_pad -edge right  -inst_name u_pwrdet8               -type pdt   -location {origin {x 2965 y 2445}}
add_pad -edge right  -signal p_ci2_8_o                  -type sig   -location {origin {x 2965 y 2450}} -bump {row 3 col 17}
add_pad -edge right  -signal p_ci2_7_o                  -type sig   -location {origin {x 2965 y 2605}} -bump {row 2 col 17}
add_pad -edge top    -inst_name u_vzz_17 -signal DVSS_0 -type vssio -location {origin {x 2795 y 2965}} -bump {row 1 col 17}
add_pad -edge top    -inst_name u_v18_17 -signal DVDD_0 -type vddio -location {origin {x 2665 y 2965}} -bump {row 2 col 16}
add_pad -edge top    -signal p_ci2_6_o                  -type sig   -location {origin {x 2635 y 2965}} -bump {row 1 col 16}
add_pad -edge top    -signal p_ci2_5_o                  -type sig   -location {origin {x 2505 y 2965}} -bump {row 2 col 15}
add_pad -edge top    -inst_name u_vdd_8 -signal VDD     -type vdd   -location {origin {x 2475 y 2965}} -bump {row 1 col 15}
add_pad -edge top    -inst_name u_vss_8 -signal VSS     -type vss   -location {origin {x 2445 y 2965}} -bump {row 3 col 15}
add_pad -edge top    -inst_name u_cbrk9                 -type cbk   -location {origin {x 2380 y 2965}}
add_pad -edge top    -signal p_ci2_v_o                  -type sig   -location {origin {x 2375 y 2965}} -bump {row 4 col 14}
add_pad -edge top    -signal p_ci2_tkn_i                -type sig   -location {origin {x 2345 y 2965}} -bump {row 2 col 14}
add_pad -edge top    -inst_name u_vzz_18 -signal DVSS_0 -type vssio -location {origin {x 2315 y 2965}} -bump {row 1 col 14}
add_pad -edge top    -inst_name u_v18_18 -signal DVDD_0 -type vddio -location {origin {x 2285 y 2965}} -bump {row 3 col 14}
add_pad -edge top    -signal p_ci2_clk_o                -type sig   -location {origin {x 2220 y 2965}} -bump {row 4 col 13}
add_pad -edge top    -signal p_ci2_4_o                  -type sig   -location {origin {x 2190 y 2965}} -bump {row 2 col 13}
add_pad -edge top    -inst_name u_pwrdet9               -type pdt   -location {origin {x 2160 y 2965}}
add_pad -edge top    -signal p_ci2_3_o                  -type sig   -location {origin {x 2155 y 2965}} -bump {row 1 col 13}
add_pad -edge top    -signal p_ci2_2_o                  -type sig   -location {origin {x 2125 y 2965}} -bump {row 3 col 13}
add_pad -edge top    -inst_name u_vzz_19 -signal DVSS_0 -type vssio -location {origin {x 2095 y 2965}} -bump {row 5 col 13}
add_pad -edge top    -inst_name u_v18_19 -signal DVDD_0 -type vddio -location {origin {x 2060 y 2965}} -bump {row 4 col 12}
add_pad -edge top    -signal p_ci2_1_o                  -type sig   -location {origin {x 2030 y 2965}} -bump {row 2 col 12}
add_pad -edge top    -signal p_ci2_0_o                  -type sig   -location {origin {x 2000 y 2965}} -bump {row 1 col 12}
add_pad -edge top    -inst_name u_cbrk11                -type cbk   -location {origin {x 1970 y 2965}}
add_pad -edge top    -signal p_core_async_reset_i       -type sig   -location {origin {x 1965 y 2965}} -bump {row 3 col 12}
add_pad -edge top    -inst_name u_vdd_9 -signal VDD     -type vdd   -location {origin {x 1935 y 2965}} -bump {row 5 col 12}
add_pad -edge top    -inst_name u_vss_9 -signal VSS     -type vss   -location {origin {x 1900 y 2965}} -bump {row 4 col 11}
add_pad -edge top    -signal p_sel_2_i                  -type sig   -location {origin {x 1870 y 2965}} -bump {row 2 col 11}
add_pad -edge top    -inst_name u_pwrdet10              -type pdt   -location {origin {x 1840 y 2965}}
add_pad -edge top    -inst_name u_v18_20 -signal DVDD_0 -type vddio -location {origin {x 1835 y 2965}} -bump {row 1 col 11}
add_pad -edge top    -inst_name u_vzz_20 -signal DVSS_0 -type vssio -location {origin {x 1805 y 2965}} -bump {row 3 col 11}
add_pad -edge top    -signal p_sel_1_i                  -type sig   -location {origin {x 1775 y 2965}} -bump {row 5 col 11}
add_pad -edge top    -signal p_sel_0_i                  -type sig   -location {origin {x 1740 y 2965}} -bump {row 4 col 10}
add_pad -edge top    -inst_name u_brk0                  -type fbk   -location {origin {x 1710 y 2965}}
add_pad -edge top    -signal p_misc_o                   -type sig   -location {origin {x 1705 y 2965}} -bump {row 2 col 10}
add_pad -edge top    -inst_name u_vss_pll -signal VSS   -type vss   -location {origin {x 1675 y 2965}} -bump {row 1 col 10}
add_pad -edge top    -signal p_clk_async_reset_i        -type sig   -location {origin {x 1645 y 2965}} -bump {row 3 col 10}
add_pad -edge top    -inst_name u_vdd_pll -signal VDD   -type vdd   -location {origin {x 1615 y 2965}} -bump {row 5 col 10}
add_pad -edge top    -inst_name u_pwrdet11              -type pdt   -location {origin {x 1685 y 2965}}
add_pad -edge top    -signal p_clk_o                    -type sig   -location {origin {x 1580 y 2965}} -bump {row 4 col 9}
add_pad -edge top    -inst_name u_vzz_21 -signal DVSS_1 -type vssio -location {origin {x 1550 y 2965}} -bump {row 2 col 9}
add_pad -edge top    -signal p_clk_C_i                  -type sig   -location {origin {x 1520 y 2965}} -bump {row 1 col 9}
add_pad -edge top    -inst_name u_v18_21 -signal DVDD_1 -type vddio -location {origin {x 1490 y 2965}} -bump {row 3 col 9}
add_pad -edge top    -inst_name u_brk1                  -type fbk   -location {origin {x 1460 y 2965}}
add_pad -edge top    -signal p_clk_B_i                  -type sig   -location {origin {x 1455 y 2965}} -bump {row 5 col 9}
add_pad -edge top    -signal p_clk_A_i                  -type sig   -location {origin {x 1415 y 2965}} -bump {row 4 col 8}
add_pad -edge top    -inst_name u_vdd_10 -signal VDD    -type vdd   -location {origin {x 1385 y 2965}} -bump {row 2 col 8}
add_pad -edge top    -inst_name u_vss_10 -signal VSS    -type vss   -location {origin {x 1355 y 2965}} -bump {row 1 col 8}
add_pad -edge top    -inst_name u_vzz_22 -signal DVSS_0 -type vssio -location {origin {x 1325 y 2965}} -bump {row 3 col 8}
add_pad -edge top    -inst_name u_v18_22 -signal DVDD_0 -type vddio -location {origin {x 1295 y 2965}} -bump {row 5 col 8}
add_pad -edge top    -signal p_co_8_i                   -type sig   -location {origin {x 1260 y 2965}} -bump {row 4 col 7}
add_pad -edge top    -signal p_co_7_i                   -type sig   -location {origin {x 1230 y 2965}} -bump {row 2 col 7}
add_pad -edge top    -inst_name u_pwrdet12              -type pdt   -location {origin {x 1200 y 2965}}
add_pad -edge top    -signal p_co_6_i                   -type sig   -location {origin {x 1195 y 2965}} -bump {row 1 col 7}
add_pad -edge top    -signal p_co_5_i                   -type sig   -location {origin {x 1165 y 2965}} -bump {row 3 col 7}
add_pad -edge top    -inst_name u_vzz_23 -signal DVSS_0 -type vssio -location {origin {x 1135 y 2965}} -bump {row 5 col 7}
add_pad -edge top    -inst_name u_v18_23 -signal DVDD_0 -type vddio -location {origin {x 1100 y 2965}} -bump {row 4 col 6}
add_pad -edge top    -signal p_co_v_i                   -type sig   -location {origin {x 1070 y 2965}} -bump {row 2 col 6}
add_pad -edge top    -signal p_co_tkn_o                 -type sig   -location {origin {x 1040 y 2965}} -bump {row 1 col 6}
add_pad -edge top    -inst_name u_cbrk12                -type cbk   -location {origin {x 1010 y 2965}}
add_pad -edge top    -signal p_co_clk_i                 -type sig   -location {origin {x 1005 y 2965}} -bump {row 3 col 6}
add_pad -edge top    -signal p_co_4_i                   -type sig   -location {origin {x  975 y 2965}} -bump {row 5 col 6}
add_pad -edge top    -inst_name u_vzz_24 -signal DVSS_0 -type vssio -location {origin {x  935 y 2965}} -bump {row 4 col 5}
add_pad -edge top    -inst_name u_v18_24 -signal DVDD_0 -type vddio -location {origin {x  905 y 2965}} -bump {row 2 col 5}
add_pad -edge top    -signal p_co_3_i                   -type sig   -location {origin {x  875 y 2965}} -bump {row 1 col 5}
add_pad -edge top    -inst_name u_vdd_11 -signal VDD    -type vdd   -location {origin {x  845 y 2965}} -bump {row 3 col 5}
add_pad -edge top    -inst_name u_vss_11 -signal VSS    -type vss   -location {origin {x  750 y 2965}} -bump {row 2 col 4}
add_pad -edge top    -signal p_co_2_i                   -type sig   -location {origin {x  720 y 2965}} -bump {row 1 col 4}
add_pad -edge top    -inst_name u_pwrdet13              -type pdt   -location {origin {x  690 y 2965}}
add_pad -edge top    -signal p_co_1_i                   -type sig   -location {origin {x  685 y 2965}} -bump {row 3 col 4}
add_pad -edge top    -signal p_co_0_i                   -type sig   -location {origin {x  585 y 2965}} -bump {row 2 col 3}
add_pad -edge top    -inst_name u_vzz_25 -signal DVSS_0 -type vssio -location {origin {x  555 y 2965}} -bump {row 1 col 3}
add_pad -edge top    -inst_name u_v18_25 -signal DVDD_0 -type vddio -location {origin {x  395 y 2965}} -bump {row 1 col 2}
add_pad -edge left   -signal p_co2_8_o                  -type sig   -location {origin {x   35 y 2795}} -bump {row 1 col 1}
add_pad -edge left   -signal p_co2_7_o                  -type sig   -location {origin {x   35 y 2670}} -bump {row 2 col 2}
add_pad -edge left   -inst_name u_cbrk13                -type cbk   -location {origin {x   35 y 2640}}
add_pad -edge left   -signal p_co2_6_o                  -type sig   -location {origin {x   35 y 2635}} -bump {row 2 col 1}
add_pad -edge left   -signal p_co2_5_o                  -type sig   -location {origin {x   35 y 2505}} -bump {row 3 col 2}
add_pad -edge left   -inst_name u_vdd_12 -signal VDD    -type vdd   -location {origin {x   35 y 2475}} -bump {row 3 col 1}
add_pad -edge left   -inst_name u_vss_12 -signal VSS    -type vss   -location {origin {x   35 y 2445}} -bump {row 3 col 3}
add_pad -edge left   -inst_name u_vzz_26 -signal DVSS_0 -type vssio -location {origin {x   35 y 2385}} -bump {row 4 col 4}
add_pad -edge left   -inst_name u_v18_26 -signal DVDD_0 -type vddio -location {origin {x   35 y 2355}} -bump {row 4 col 2}
add_pad -edge left   -signal p_co2_v_o                  -type sig   -location {origin {x   35 y 2325}} -bump {row 4 col 1}
add_pad -edge left   -signal p_co2_tkn_i                -type sig   -location {origin {x   35 y 2295}} -bump {row 4 col 3}
add_pad -edge left   -inst_name u_pwrdet14              -type pdt   -location {origin {x   35 y 2220}}
add_pad -edge left   -signal p_co2_clk_o                -type sig   -location {origin {x   35 y 2215}} -bump {row 5 col 4}
add_pad -edge left   -signal p_co2_4_o                  -type sig   -location {origin {x   35 y 2185}} -bump {row 5 col 2}
add_pad -edge left   -inst_name u_vzz_27 -signal DVSS_0 -type vssio -location {origin {x   35 y 2155}} -bump {row 5 col 1}
add_pad -edge left   -inst_name u_v18_27 -signal DVDD_0 -type vddio -location {origin {x   35 y 2125}} -bump {row 5 col 3}
add_pad -edge left   -signal p_co2_3_o                  -type sig   -location {origin {x   35 y 2095}} -bump {row 5 col 5}
add_pad -edge left   -signal p_co2_2_o                  -type sig   -location {origin {x   35 y 2060}} -bump {row 6 col 4}
add_pad -edge left   -inst_name u_cbrk15                -type cbk   -location {origin {x   35 y 2030}}
add_pad -edge left   -signal p_co2_1_o                  -type sig   -location {origin {x   35 y 2025}} -bump {row 6 col 2}
add_pad -edge left   -signal p_co2_0_o                  -type sig   -location {origin {x   35 y 1995}} -bump {row 6 col 1}
add_pad -edge left   -inst_name u_vzz_28 -signal DVSS_0 -type vssio -location {origin {x   35 y 1965}} -bump {row 6 col 3}
add_pad -edge left   -inst_name u_v18_28 -signal DVDD_0 -type vddio -location {origin {x   35 y 1935}} -bump {row 6 col 5}
add_pad -edge left   -signal p_bsg_tag_clk_o            -type sig   -location {origin {x   35 y 1900}} -bump {row 7 col 4}
add_pad -edge left   -inst_name u_vdd_13 -signal VDD    -type vdd   -location {origin {x   35 y 1870}} -bump {row 7 col 2}
add_pad -edge left   -inst_name u_vss_13 -signal VSS    -type vss   -location {origin {x   35 y 1840}} -bump {row 7 col 1}
add_pad -edge left   -signal p_bsg_tag_data_o           -type sig   -location {origin {x   35 y 1810}} -bump {row 7 col 3}
add_pad -edge left   -inst_name u_pwrdet15              -type pdt   -location {origin {x   35 y 1780}}
add_pad -edge left   -signal p_ddr_dq_7_io              -type sig   -location {origin {x   35 y 1775}} -bump {row 7 col 5}
add_pad -edge left   -signal p_ddr_dq_6_io              -type sig   -location {origin {x   35 y 1735}} -bump {row 8 col 4}
add_pad -edge left   -inst_name u_vzz_29 -signal DVSS_0 -type vssio -location {origin {x   35 y 1705}} -bump {row 8 col 2}
add_pad -edge left   -inst_name u_v18_29 -signal DVDD_0 -type vddio -location {origin {x   35 y 1675}} -bump {row 8 col 1}
add_pad -edge left   -signal p_ddr_dq_5_io              -type sig   -location {origin {x   35 y 1645}} -bump {row 8 col 3}
add_pad -edge left   -signal p_ddr_dq_4_io              -type sig   -location {origin {x   35 y 1615}} -bump {row 8 col 5}
add_pad -edge left   -inst_name u_cbrk16                -type cbk   -location {origin {x   35 y 1580}}
add_pad -edge left   -signal p_ddr_dq_3_io              -type sig   -location {origin {x   35 y 1575}} -bump {row 9 col 4}
add_pad -edge left   -signal p_ddr_dq_2_io              -type sig   -location {origin {x   35 y 1545}} -bump {row 9 col 2}
add_pad -edge left   -inst_name u_vzz_30 -signal DVSS_0 -type vssio -location {origin {x   35 y 1515}} -bump {row 9 col 1}
add_pad -edge left   -inst_name u_v18_30 -signal DVDD_0 -type vddio -location {origin {x   35 y 1485}} -bump {row 9 col 3}
add_pad -edge left   -signal p_ddr_dq_1_io              -type sig   -location {origin {x   35 y 1455}} -bump {row 9 col 5}
add_pad -edge left   -signal p_ddr_dq_0_io              -type sig   -location {origin {x   35 y 1420}} -bump {row 10 col 4}
add_pad -edge left   -inst_name u_vdd_14 -signal VDD    -type vdd   -location {origin {x   35 y 1390}} -bump {row 10 col 2}
add_pad -edge left   -inst_name u_vss_14 -signal VSS    -type vss   -location {origin {x   35 y 1360}} -bump {row 10 col 1}
add_pad -edge left   -inst_name u_pwrdet16              -type pdt   -location {origin {x   35 y 1330}}
add_pad -edge left   -signal p_ddr_dm_0_o               -type sig   -location {origin {x   35 y 1325}} -bump {row 10 col 3}
add_pad -edge left   -signal p_ddr_dqs_n_0_io           -type sig   -location {origin {x   35 y 1295}} -bump {row 10 col 5}
add_pad -edge left   -inst_name u_vzz_31 -signal DVSS_0 -type vssio -location {origin {x   35 y 1260}} -bump {row 11 col 4}
add_pad -edge left   -inst_name u_v18_31 -signal DVDD_0 -type vddio -location {origin {x   35 y 1230}} -bump {row 11 col 2}
add_pad -edge left   -signal p_ddr_dqs_p_0_io           -type sig   -location {origin {x   35 y 1200}} -bump {row 11 col 1}
add_pad -edge left   -signal p_ddr_dq_15_io             -type sig   -location {origin {x   35 y 1170}} -bump {row 11 col 3}
add_pad -edge left   -inst_name u_cbrk17                -type cbk   -location {origin {x   35 y 1140}}
add_pad -edge left   -signal p_ddr_dq_14_io             -type sig   -location {origin {x   35 y 1135}} -bump {row 11 col 5}
add_pad -edge left   -signal p_ddr_dq_13_io             -type sig   -location {origin {x   35 y 1095}} -bump {row 12 col 4}
add_pad -edge left   -inst_name u_vzz_32 -signal DVSS_0 -type vssio -location {origin {x   35 y 1065}} -bump {row 12 col 2}
add_pad -edge left   -inst_name u_v18_32 -signal DVDD_0 -type vddio -location {origin {x   35 y 1035}} -bump {row 12 col 1}
add_pad -edge left   -signal p_ddr_dq_12_io             -type sig   -location {origin {x   35 y 1005}} -bump {row 12 col 3}
add_pad -edge left   -signal p_ddr_dq_11_io             -type sig   -location {origin {x   35 y  975}} -bump {row 12 col 5}
add_pad -edge left   -inst_name u_pwrdet17              -type pdt   -location {origin {x   35 y  940}}
add_pad -edge left   -signal p_ddr_dq_10_io             -type sig   -location {origin {x   35 y  935}} -bump {row 13 col 4}
add_pad -edge left   -signal p_ddr_dq_9_io              -type sig   -location {origin {x   35 y  905}} -bump {row 13 col 2}
add_pad -edge left   -signal p_ddr_dq_8_io              -type sig   -location {origin {x   35 y  875}} -bump {row 13 col 1}
add_pad -edge left   -inst_name u_vdd_15 -signal VDD    -type vdd   -location {origin {x   35 y  845}} -bump {row 13 col 3}
add_pad -edge left   -inst_name u_vss_15 -signal VSS    -type vss   -location {origin {x   35 y  745}} -bump {row 14 col 2}
add_pad -edge left   -inst_name u_vzz_33 -signal DVSS_0 -type vssio -location {origin {x   35 y  715}} -bump {row 14 col 1}
add_pad -edge left   -inst_name u_v18_33 -signal DVDD_0 -type vddio -location {origin {x   35 y  685}} -bump {row 14 col 3}
add_pad -edge left   -signal UNASSIGNED                 -type sig   -location {origin {x   35 y  555}} -bump {row 15 col 2}
add_pad -edge left   -signal UNASSIGNED                 -type sig   -location {origin {x   35 y  525}} -bump {row 15 col 1}
add_pad -edge left   -signal UNASSIGNED                 -type sig   -location {origin {x   35 y  395}} -bump {row 16 col 1}

puts "Trigger errors for incorrect bump options"
set_bump_options -rdl_layer metal10 -rdl_width 0.5 -rdl_spacing 1
catch {initialize_padring}
set_bump_options -rdl_layer metal10 -rdl_width 1.0 -rdl_spacing 0.5
catch {initialize_padring}

puts "Reset bump options"
set_bump_options -rdl_layer metal10 -rdl_width 10  -rdl_spacing 10

if {[catch {initialize_padring} msg]} {
  puts $errorInfo
  puts $msg
  return
}

set def_file1 [make_result_file "tcl_interface.flipchip.1.def"]
set def_file  [make_result_file "tcl_interface.flipchip.def"]

write_def $def_file1
exec sed -e "/END SPECIALNETS/r[ICeWall::get_footprint_rdl_cover_file_name]" $def_file1 > $def_file
diff_files $def_file "tcl_interface.flipchip.defok"

