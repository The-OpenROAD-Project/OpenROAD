source "helpers.tcl"

read_lef NangateOpenCellLibrary.mod.lef
read_lef dummy_pads.lef

read_liberty dummy_pads.lib


# Load library defintions
define_pad_cell \
  -name PAD \
  -orient {bottom R0 right R90 top R180 left R270} \
  -bondpad \
  -pad_pin_name PAD

catch {define_pad_cell \
  -name PADCELL_SIG \
  type sig \
  -cell_name {top PADCELL_SIG_V bottom PADCELL_SIG_V left PADCELL_SIG_H right PADCELL_SIG_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name PAD
}

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
  -fill \
  -cell_name {bottom PAD_FILL5_V right PAD_FILL5_H top PAD_FILL5_V left PAD_FILL5_H} \
  -orient {bottom R0 right MY top R180 left MX} \
  -physical_only

define_pad_cell \
  -name PAD_FILL1 \
  -fill \
  -cell_name {bottom PAD_FILL1_V right PAD_FILL1_H top PAD_FILL1_V left PAD_FILL1_H} \
  -orient {bottom R0 right MY top R180 left MX} \
  -physical_only

define_pad_cell \
  -name PAD_CORNER \
  -corner \
  -orient {ll R0 lr R90 ur R180 ul R270} \
  -physical_only

define_pad_cell \
  -name DUMMY_BUMP \
  -bump \
  -physical_only

# Test help messages
help define_pad_cell
help set_bump_options
help set_padring_options
help place_cell
help add_pad
help initialize_padring

read_verilog soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.v

link_design soc_bsg_black_parrot

catch {set_padring_options -type wirebond -power_nets  {VDD DVDD_0 DVDD_1} -ground_nets {VSS DVSS_0 DVSS_1} -offsets 35 -pin_layer metal10 -pad_inst_pattern "%s"}
catch {set_padring_options -type wirebond -offsets {35 36 37} -pin_layer metal10 -pad_inst_pattern "%s"}

catch {set_padring_options \
  -type wirebond \
  -power  {VDD DVDD_0 DVDD_1} \
  -ground {VSS DVSS_0 DVSS_1} \
  offsets 35 \
  -pin_layer metal10 \
  -pad_inst_pattern "%s" \
  -pad_pin_pattern "p_%s" \
  -connect_by_abutment {SNS RETN DVDD DVSS}
}

set_padring_options \
  -type wirebond \
  -power  {VDD DVDD_0 DVDD_1} \
  -ground {VSS DVSS_0 DVSS_1} \
  -offsets 35 \
  -pin_layer metal10 \
  -pad_inst_pattern "%s" \
  -connect_by_abutment {SNS RETN DVDD DVSS}

place_cell -cell MARKER -inst_name u_marker_0 -origin {1197.5 1199.3} -orient R0 -status FIRM

puts "Trigger errors"
# Trigger errors
catch {add_pad -edge other  -signal p_ddr_dm_1_o         -type sig   -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal dr_dm_1_o            -type sig   -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sign  -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location somewhere                        -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center}                         -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {origin}                         -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {292.000 105.000}}       -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {X  292.000 Y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {x  292.000 Y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {X  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}} -bump not_allowed_for_wirebond}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {origin}                         -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {origin {292.000 105.000}}       -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {origin {X  292.000 Y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {x  292.000 Y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {X  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {x  292.000 y  105.000} orient no} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -cell SIG         -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -cell PADCELL_VDD -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -cell PADCELL_SIG -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}

puts "This one is correct"
add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -cell PADCELL_SIG_V -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}

puts "Detect duplicates"
catch {add_pad -edge bottom -signal p_ddr_dm_1_o         -type sig   -location {center {x  292.000 y  105.000}} -bondpad {center {x  292.000 y   63.293}}}
catch {add_pad -edge bottom -signal p_ddr_dqs_n_1_io     -type sig   -location {center {x  362.000 y  105.000}} bondpad {center {x  362.000 y   63.293}}}

puts "No more errors expected"

# Define the same padring for soc_bsg_black_parrot_nangate45 using TCL commands, rather than strategy file.
#
add_pad -edge bottom -signal p_ddr_dqs_n_1_io     -type sig   -location {center {x  362.000 y  105.000}} -bondpad {center {x  362.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_dqs_p_1_io     -type sig   -location {center {x  432.000 y  105.000}} -bondpad {center {x  432.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_ba_2_o         -type sig   -location {center {x  502.000 y  105.000}} -bondpad {center {x  502.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_ba_1_o         -type sig   -location {center {x  607.000 y  105.000}} -bondpad {center {x  607.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_ba_0_o         -type sig   -location {center {x  677.000 y  105.000}} -bondpad {center {x  677.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_15_o      -type sig   -location {center {x  747.000 y  105.000}} -bondpad {center {x  747.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_14_o      -type sig   -location {center {x  817.000 y  105.000}} -bondpad {center {x  817.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_13_o      -type sig   -location {center {x  887.000 y  105.000}} -bondpad {center {x  887.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_12_o      -type sig   -location {center {x  957.000 y  105.000}} -bondpad {center {x  957.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_11_o      -type sig   -location {center {x 1027.000 y  105.000}} -bondpad {center {x 1027.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_10_o      -type sig   -location {center {x 1097.000 y  105.000}} -bondpad {center {x 1097.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_9_o       -type sig   -location {center {x 1167.000 y  105.000}} -bondpad {center {x 1167.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_8_o       -type sig   -location {center {x 1272.000 y  105.000}} -bondpad {center {x 1272.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_7_o       -type sig   -location {center {x 1342.000 y  105.000}} -bondpad {center {x 1342.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_6_o       -type sig   -location {center {x 1412.000 y  105.000}} -bondpad {center {x 1412.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_5_o       -type sig   -location {center {x 1482.000 y  105.000}} -bondpad {center {x 1482.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_4_o       -type sig   -location {center {x 1552.000 y  105.000}} -bondpad {center {x 1552.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_3_o       -type sig   -location {center {x 1622.000 y  105.000}} -bondpad {center {x 1622.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_2_o       -type sig   -location {center {x 1692.000 y  105.000}} -bondpad {center {x 1692.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_1_o       -type sig   -location {center {x 1762.000 y  105.000}} -bondpad {center {x 1762.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_addr_0_o       -type sig   -location {center {x 1867.000 y  105.000}} -bondpad {center {x 1867.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_odt_o          -type sig   -location {center {x 1937.000 y  105.000}} -bondpad {center {x 1937.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_reset_n_o      -type sig   -location {center {x 2007.000 y  105.000}} -bondpad {center {x 2007.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_we_n_o         -type sig   -location {center {x 2077.000 y  105.000}} -bondpad {center {x 2077.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_cas_n_o        -type sig   -location {center {x 2147.000 y  105.000}} -bondpad {center {x 2147.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_ras_n_o        -type sig   -location {center {x 2217.000 y  105.000}} -bondpad {center {x 2217.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_cs_n_o         -type sig   -location {center {x 2287.000 y  105.000}} -bondpad {center {x 2287.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_cke_o          -type sig   -location {center {x 2357.000 y  105.000}} -bondpad {center {x 2357.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_ck_n_o         -type sig   -location {center {x 2427.000 y  105.000}} -bondpad {center {x 2427.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_ck_p_o         -type sig   -location {center {x 2532.000 y  105.000}} -bondpad {center {x 2532.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_dqs_n_2_io     -type sig   -location {center {x 2602.000 y  105.000}} -bondpad {center {x 2602.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_dqs_p_2_io     -type sig   -location {center {x 2672.000 y  105.000}} -bondpad {center {x 2672.000 y   63.293}}
add_pad -edge bottom -signal p_ddr_dm_2_o         -type sig   -location {center {x 2742.000 y  105.000}} -bondpad {center {x 2742.000 y   63.293}}
add_pad -edge bottom -inst_name u_vzz_0   -signal DVSS_0               -type vssio -location {center {x  257.000 y  105.000}} -bondpad {center {x  257.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_0   -signal DVDD_0               -type vddio -location {center {x  327.000 y  105.000}} -bondpad {center {x  327.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_0   -signal VSS                  -type vss   -location {center {x  397.000 y  105.000}} -bondpad {center {x  397.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_0   -signal VDD                  -type vdd   -location {center {x  467.000 y  105.000}} -bondpad {center {x  467.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_1   -signal DVSS_0               -type vssio -location {center {x  537.000 y  105.000}} -bondpad {center {x  537.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_1   -signal DVDD_0               -type vddio -location {center {x  642.000 y  105.000}} -bondpad {center {x  642.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_1   -signal VSS                  -type vss   -location {center {x  712.000 y  105.000}} -bondpad {center {x  712.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_1   -signal VDD                  -type vdd   -location {center {x  782.000 y  105.000}} -bondpad {center {x  782.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_2   -signal DVSS_0               -type vssio -location {center {x  852.000 y  105.000}} -bondpad {center {x  852.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_2   -signal DVDD_0               -type vddio -location {center {x  922.000 y  105.000}} -bondpad {center {x  922.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_2   -signal VSS                  -type vss   -location {center {x  992.000 y  105.000}} -bondpad {center {x  992.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_2   -signal VDD                  -type vdd   -location {center {x 1062.000 y  105.000}} -bondpad {center {x 1062.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_3   -signal DVSS_0               -type vssio -location {center {x 1132.000 y  105.000}} -bondpad {center {x 1132.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_3   -signal DVDD_0               -type vddio -location {center {x 1237.000 y  105.000}} -bondpad {center {x 1237.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_3   -signal VSS                  -type vss   -location {center {x 1307.000 y  105.000}} -bondpad {center {x 1307.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_3   -signal VDD                  -type vdd   -location {center {x 1377.000 y  105.000}} -bondpad {center {x 1377.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_4   -signal DVSS_0               -type vssio -location {center {x 1447.000 y  105.000}} -bondpad {center {x 1447.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_4   -signal DVDD_0               -type vddio -location {center {x 1517.000 y  105.000}} -bondpad {center {x 1517.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_4   -signal VSS                  -type vss   -location {center {x 1587.000 y  105.000}} -bondpad {center {x 1587.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_4   -signal VDD                  -type vdd   -location {center {x 1657.000 y  105.000}} -bondpad {center {x 1657.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_5   -signal DVSS_0               -type vssio -location {center {x 1727.000 y  105.000}} -bondpad {center {x 1727.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_5   -signal DVDD_0               -type vddio -location {center {x 1797.000 y  105.000}} -bondpad {center {x 1797.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_5   -signal VSS                  -type vss   -location {center {x 1902.000 y  105.000}} -bondpad {center {x 1902.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_5   -signal VDD                  -type vdd   -location {center {x 1972.000 y  105.000}} -bondpad {center {x 1972.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_6   -signal DVSS_0               -type vssio -location {center {x 2042.000 y  105.000}} -bondpad {center {x 2042.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_6   -signal DVDD_0               -type vddio -location {center {x 2112.000 y  105.000}} -bondpad {center {x 2112.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_6   -signal VSS                  -type vss   -location {center {x 2182.000 y  105.000}} -bondpad {center {x 2182.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_6   -signal VDD                  -type vdd   -location {center {x 2252.000 y  105.000}} -bondpad {center {x 2252.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_7   -signal DVSS_0               -type vssio -location {center {x 2322.000 y  105.000}} -bondpad {center {x 2322.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_7   -signal DVDD_0               -type vddio -location {center {x 2392.000 y  105.000}} -bondpad {center {x 2392.000 y  149.893}}
add_pad -edge bottom -inst_name u_vss_7   -signal VSS                  -type vss   -location {center {x 2497.000 y  105.000}} -bondpad {center {x 2497.000 y  149.893}}
add_pad -edge bottom -inst_name u_vdd_7   -signal VDD                  -type vdd   -location {center {x 2567.000 y  105.000}} -bondpad {center {x 2567.000 y  149.893}}
add_pad -edge bottom -inst_name u_vzz_8   -signal DVSS_0               -type vssio -location {center {x 2637.000 y  105.000}} -bondpad {center {x 2637.000 y  149.893}}
add_pad -edge bottom -inst_name u_v18_8   -signal DVDD_0               -type vddio -location {center {x 2707.000 y  105.000}} -bondpad {center {x 2707.000 y  149.893}}
add_pad -edge bottom -inst_name u_cbrk0                -type cbk   -location {center {x  239.500 y  105.000}}
add_pad -edge bottom -inst_name u_cbrk1                -type cbk   -location {center {x  869.500 y  105.000}}
add_pad -edge bottom -inst_name u_cbrk2                -type cbk   -location {center {x 1499.500 y  105.000}}
add_pad -edge bottom -inst_name u_cbrk3                -type cbk   -location {center {x 2129.500 y  105.000}}
add_pad -edge bottom -inst_name u_cbrk4                -type cbk   -location {center {x 2759.500 y  105.000}}
add_pad -edge bottom -inst_name u_pwrdet0              -type pdt   -location {center {x  572.000 y  105.000}}
add_pad -edge bottom -inst_name u_pwrdet1              -type pdt   -location {center {x 1202.000 y  105.000}}
add_pad -edge bottom -inst_name u_pwrdet2              -type pdt   -location {center {x 1832.000 y  105.000}}
add_pad -edge bottom -inst_name u_pwrdet3              -type pdt   -location {center {x 2462.000 y  105.000}}
add_pad -edge right  -signal p_ddr_dq_23_io       -type sig   -location {center {x 2895.000 y  292.000}} -bondpad {center {x 2936.707 y  292.000}}
add_pad -edge right  -signal p_ddr_dq_22_io       -type sig   -location {center {x 2895.000 y  362.000}} -bondpad {center {x 2936.707 y  362.000}}
add_pad -edge right  -signal p_ddr_dq_21_io       -type sig   -location {center {x 2895.000 y  432.000}} -bondpad {center {x 2936.707 y  432.000}}
add_pad -edge right  -signal p_ddr_dq_20_io       -type sig   -location {center {x 2895.000 y  502.000}} -bondpad {center {x 2936.707 y  502.000}}
add_pad -edge right  -signal p_ddr_dq_19_io       -type sig   -location {center {x 2895.000 y  607.000}} -bondpad {center {x 2936.707 y  607.000}}
add_pad -edge right  -signal p_ddr_dq_18_io       -type sig   -location {center {x 2895.000 y  677.000}} -bondpad {center {x 2936.707 y  677.000}}
add_pad -edge right  -signal p_ddr_dq_17_io       -type sig   -location {center {x 2895.000 y  747.000}} -bondpad {center {x 2936.707 y  747.000}}
add_pad -edge right  -signal p_ddr_dq_16_io       -type sig   -location {center {x 2895.000 y  817.000}} -bondpad {center {x 2936.707 y  817.000}}
add_pad -edge right  -signal p_ddr_dq_31_io       -type sig   -location {center {x 2895.000 y  887.000}} -bondpad {center {x 2936.707 y  887.000}}
add_pad -edge right  -signal p_ddr_dq_30_io       -type sig   -location {center {x 2895.000 y  957.000}} -bondpad {center {x 2936.707 y  957.000}}
add_pad -edge right  -signal p_ddr_dq_29_io       -type sig   -location {center {x 2895.000 y 1027.000}} -bondpad {center {x 2936.707 y 1027.000}}
add_pad -edge right  -signal p_ddr_dq_28_io       -type sig   -location {center {x 2895.000 y 1097.000}} -bondpad {center {x 2936.707 y 1097.000}}
add_pad -edge right  -signal p_ddr_dq_27_io       -type sig   -location {center {x 2895.000 y 1167.000}} -bondpad {center {x 2936.707 y 1167.000}}
add_pad -edge right  -signal p_ddr_dq_26_io       -type sig   -location {center {x 2895.000 y 1272.000}} -bondpad {center {x 2936.707 y 1272.000}}
add_pad -edge right  -signal p_ddr_dq_25_io       -type sig   -location {center {x 2895.000 y 1342.000}} -bondpad {center {x 2936.707 y 1342.000}}
add_pad -edge right  -signal p_ddr_dq_24_io       -type sig   -location {center {x 2895.000 y 1412.000}} -bondpad {center {x 2936.707 y 1412.000}}
add_pad -edge right  -signal p_ddr_dqs_n_3_io     -type sig   -location {center {x 2895.000 y 1482.000}} -bondpad {center {x 2936.707 y 1482.000}}
add_pad -edge right  -signal p_ddr_dqs_p_3_io     -type sig   -location {center {x 2895.000 y 1552.000}} -bondpad {center {x 2936.707 y 1552.000}}
add_pad -edge right  -signal p_ddr_dm_3_o         -type sig   -location {center {x 2895.000 y 1622.000}} -bondpad {center {x 2936.707 y 1622.000}}
add_pad -edge right  -signal p_bsg_tag_clk_i      -type sig   -location {center {x 2895.000 y 1692.000}} -bondpad {center {x 2936.707 y 1692.000}}
add_pad -edge right  -signal p_bsg_tag_data_i     -type sig   -location {center {x 2895.000 y 1762.000}} -bondpad {center {x 2936.707 y 1762.000}}
add_pad -edge right  -signal p_bsg_tag_en_i       -type sig   -location {center {x 2895.000 y 1867.000}} -bondpad {center {x 2936.707 y 1867.000}}
add_pad -edge right  -signal p_ci_8_i             -type sig   -location {center {x 2895.000 y 1937.000}} -bondpad {center {x 2936.707 y 1937.000}}
add_pad -edge right  -signal p_ci_7_i             -type sig   -location {center {x 2895.000 y 2007.000}} -bondpad {center {x 2936.707 y 2007.000}}
add_pad -edge right  -signal p_ci_6_i             -type sig   -location {center {x 2895.000 y 2077.000}} -bondpad {center {x 2936.707 y 2077.000}}
add_pad -edge right  -signal p_ci_5_i             -type sig   -location {center {x 2895.000 y 2147.000}} -bondpad {center {x 2936.707 y 2147.000}}
add_pad -edge right  -signal p_ci_v_i             -type sig   -location {center {x 2895.000 y 2217.000}} -bondpad {center {x 2936.707 y 2217.000}}
add_pad -edge right  -signal p_ci_tkn_o           -type sig   -location {center {x 2895.000 y 2287.000}} -bondpad {center {x 2936.707 y 2287.000}}
add_pad -edge right  -signal p_ci_clk_i           -type sig   -location {center {x 2895.000 y 2357.000}} -bondpad {center {x 2936.707 y 2357.000}}
add_pad -edge right  -signal p_ci_4_i             -type sig   -location {center {x 2895.000 y 2427.000}} -bondpad {center {x 2936.707 y 2427.000}}
add_pad -edge right  -signal p_ci_3_i             -type sig   -location {center {x 2895.000 y 2532.000}} -bondpad {center {x 2936.707 y 2532.000}}
add_pad -edge right  -signal p_ci_2_i             -type sig   -location {center {x 2895.000 y 2602.000}} -bondpad {center {x 2936.707 y 2602.000}}
add_pad -edge right  -signal p_ci_1_i             -type sig   -location {center {x 2895.000 y 2672.000}} -bondpad {center {x 2936.707 y 2672.000}}
add_pad -edge right  -signal p_ci_0_i             -type sig   -location {center {x 2895.000 y 2742.000}} -bondpad {center {x 2936.707 y 2742.000}}
add_pad -edge right  -inst_name u_vss_8   -signal VSS                  -type vss   -location {center {x 2895.000 y  257.000}} -bondpad {center {x 2850.161 y  257.000}}
add_pad -edge right  -inst_name u_vdd_8   -signal VDD                  -type vdd   -location {center {x 2895.000 y  327.000}} -bondpad {center {x 2850.161 y  327.000}}
add_pad -edge right  -inst_name u_vzz_9   -signal DVSS_0               -type vssio -location {center {x 2895.000 y  397.000}} -bondpad {center {x 2850.161 y  397.000}}
add_pad -edge right  -inst_name u_v18_9   -signal DVDD_0               -type vddio -location {center {x 2895.000 y  467.000}} -bondpad {center {x 2850.161 y  467.000}}
add_pad -edge right  -inst_name u_vss_9   -signal VSS                  -type vss   -location {center {x 2895.000 y  537.000}} -bondpad {center {x 2850.161 y  537.000}}
add_pad -edge right  -inst_name u_vdd_9   -signal VDD                  -type vdd   -location {center {x 2895.000 y  642.000}} -bondpad {center {x 2850.161 y  642.000}}
add_pad -edge right  -inst_name u_vzz_10  -signal DVSS_0               -type vssio -location {center {x 2895.000 y  712.000}} -bondpad {center {x 2850.161 y  712.000}}
add_pad -edge right  -inst_name u_v18_10  -signal DVDD_0               -type vddio -location {center {x 2895.000 y  782.000}} -bondpad {center {x 2850.161 y  782.000}}
add_pad -edge right  -inst_name u_vss_10  -signal VSS                  -type vss   -location {center {x 2895.000 y  852.000}} -bondpad {center {x 2850.161 y  852.000}}
add_pad -edge right  -inst_name u_vdd_10  -signal VDD                  -type vdd   -location {center {x 2895.000 y  922.000}} -bondpad {center {x 2850.161 y  922.000}}
add_pad -edge right  -inst_name u_vzz_11  -signal DVSS_0               -type vssio -location {center {x 2895.000 y  992.000}} -bondpad {center {x 2850.161 y  992.000}}
add_pad -edge right  -inst_name u_v18_11  -signal DVDD_0               -type vddio -location {center {x 2895.000 y 1062.000}} -bondpad {center {x 2850.161 y 1062.000}}
add_pad -edge right  -inst_name u_vss_11  -signal VSS                  -type vss   -location {center {x 2895.000 y 1132.000}} -bondpad {center {x 2850.161 y 1132.000}}
add_pad -edge right  -inst_name u_vdd_11  -signal VDD                  -type vdd   -location {center {x 2895.000 y 1237.000}} -bondpad {center {x 2850.161 y 1237.000}}
add_pad -edge right  -inst_name u_vzz_12  -signal DVSS_0               -type vssio -location {center {x 2895.000 y 1307.000}} -bondpad {center {x 2850.161 y 1307.000}}
add_pad -edge right  -inst_name u_v18_12  -signal DVDD_0               -type vddio -location {center {x 2895.000 y 1377.000}} -bondpad {center {x 2850.161 y 1377.000}}
add_pad -edge right  -inst_name u_vss_12  -signal VSS                  -type vss   -location {center {x 2895.000 y 1447.000}} -bondpad {center {x 2850.161 y 1447.000}}
add_pad -edge right  -inst_name u_vdd_12  -signal VDD                  -type vdd   -location {center {x 2895.000 y 1517.000}} -bondpad {center {x 2850.161 y 1517.000}}
add_pad -edge right  -inst_name u_vzz_13  -signal DVSS_0               -type vssio -location {center {x 2895.000 y 1587.000}} -bondpad {center {x 2850.161 y 1587.000}}
add_pad -edge right  -inst_name u_v18_13  -signal DVDD_0               -type vddio -location {center {x 2895.000 y 1657.000}} -bondpad {center {x 2850.161 y 1657.000}}
add_pad -edge right  -inst_name u_vss_13  -signal VSS                  -type vss   -location {center {x 2895.000 y 1727.000}} -bondpad {center {x 2850.161 y 1727.000}}
add_pad -edge right  -inst_name u_vdd_13  -signal VDD                  -type vdd   -location {center {x 2895.000 y 1797.000}} -bondpad {center {x 2850.161 y 1797.000}}
add_pad -edge right  -inst_name u_vzz_14  -signal DVSS_0               -type vssio -location {center {x 2895.000 y 1902.000}} -bondpad {center {x 2850.161 y 1902.000}}
add_pad -edge right  -inst_name u_v18_14  -signal DVDD_0               -type vddio -location {center {x 2895.000 y 1972.000}} -bondpad {center {x 2850.161 y 1972.000}}
add_pad -edge right  -inst_name u_vss_14  -signal VSS                  -type vss   -location {center {x 2895.000 y 2042.000}} -bondpad {center {x 2850.161 y 2042.000}}
add_pad -edge right  -inst_name u_vdd_14  -signal VDD                  -type vdd   -location {center {x 2895.000 y 2112.000}} -bondpad {center {x 2850.161 y 2112.000}}
add_pad -edge right  -inst_name u_vzz_15  -signal DVSS_0               -type vssio -location {center {x 2895.000 y 2182.000}} -bondpad {center {x 2850.161 y 2182.000}}
add_pad -edge right  -inst_name u_v18_15  -signal DVDD_0               -type vddio -location {center {x 2895.000 y 2252.000}} -bondpad {center {x 2850.161 y 2252.000}}
add_pad -edge right  -inst_name u_vss_15  -signal VSS                  -type vss   -location {center {x 2895.000 y 2322.000}} -bondpad {center {x 2850.161 y 2322.000}}
add_pad -edge right  -inst_name u_vdd_15  -signal VDD                  -type vdd   -location {center {x 2895.000 y 2392.000}} -bondpad {center {x 2850.161 y 2392.000}}
add_pad -edge right  -inst_name u_vzz_16  -signal DVSS_0               -type vssio -location {center {x 2895.000 y 2497.000}} -bondpad {center {x 2850.161 y 2497.000}}
add_pad -edge right  -inst_name u_v18_16  -signal DVDD_0               -type vddio -location {center {x 2895.000 y 2567.000}} -bondpad {center {x 2850.161 y 2567.000}}
add_pad -edge right  -inst_name u_vss_16  -signal VSS                  -type vss   -location {center {x 2895.000 y 2637.000}} -bondpad {center {x 2850.161 y 2637.000}}
add_pad -edge right  -inst_name u_vdd_16  -signal VDD                  -type vdd   -location {center {x 2895.000 y 2707.000}} -bondpad {center {x 2850.161 y 2707.000}}
add_pad -edge right  -inst_name u_cbrk5                -type cbk   -location {center {x 2895.000 y  239.500}}
add_pad -edge right  -inst_name u_cbrk6                -type cbk   -location {center {x 2895.000 y  869.500}}
add_pad -edge right  -inst_name u_cbrk7                -type cbk   -location {center {x 2895.000 y 1499.500}}
add_pad -edge right  -inst_name u_cbrk8                -type cbk   -location {center {x 2895.000 y 2129.500}}
add_pad -edge right  -inst_name u_cbrk9                -type cbk   -location {center {x 2895.000 y 2759.500}}
add_pad -edge right  -inst_name u_pwrdet4              -type pdt   -location {center {x 2895.000 y  572.000}}
add_pad -edge right  -inst_name u_pwrdet5              -type pdt   -location {center {x 2895.000 y 1202.000}}
add_pad -edge right  -inst_name u_pwrdet6              -type pdt   -location {center {x 2895.000 y 1832.000}}
add_pad -edge right  -inst_name u_pwrdet7              -type pdt   -location {center {x 2895.000 y 2462.000}}
add_pad -edge top    -signal p_ci2_8_o            -type sig   -location {center {x 2760.000 y 2895.000}} -bondpad {center {x 2760.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_7_o            -type sig   -location {center {x 2690.000 y 2895.000}} -bondpad {center {x 2690.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_6_o            -type sig   -location {center {x 2620.000 y 2895.000}} -bondpad {center {x 2620.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_5_o            -type sig   -location {center {x 2550.000 y 2895.000}} -bondpad {center {x 2550.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_v_o            -type sig   -location {center {x 2445.000 y 2895.000}} -bondpad {center {x 2445.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_tkn_i          -type sig   -location {center {x 2375.000 y 2895.000}} -bondpad {center {x 2375.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_clk_o          -type sig   -location {center {x 2305.000 y 2895.000}} -bondpad {center {x 2305.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_4_o            -type sig   -location {center {x 2235.000 y 2895.000}} -bondpad {center {x 2235.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_3_o            -type sig   -location {center {x 2165.000 y 2895.000}} -bondpad {center {x 2165.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_2_o            -type sig   -location {center {x 2095.000 y 2895.000}} -bondpad {center {x 2095.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_1_o            -type sig   -location {center {x 2025.000 y 2895.000}} -bondpad {center {x 2025.000 y 2850.161}}
add_pad -edge top    -signal p_ci2_0_o            -type sig   -location {center {x 1955.000 y 2895.000}} -bondpad {center {x 1955.000 y 2850.161}}
add_pad -edge top    -signal p_core_async_reset_i -type sig   -location {center {x 1850.000 y 2895.000}} -bondpad {center {x 1850.000 y 2850.161}}
add_pad -edge top    -signal p_sel_2_i            -type sig   -location {center {x 1780.000 y 2895.000}} -bondpad {center {x 1780.000 y 2850.161}}
add_pad -edge top    -signal p_sel_1_i            -type sig   -location {center {x 1710.000 y 2895.000}} -bondpad {center {x 1710.000 y 2850.161}}
add_pad -edge top    -signal p_sel_0_i            -type sig   -location {center {x 1640.000 y 2895.000}} -bondpad {center {x 1640.000 y 2850.161}}
add_pad -edge top    -signal p_misc_o             -type sig   -location {center {x 1570.000 y 2895.000}} -bondpad {center {x 1570.000 y 2850.161}}
add_pad -edge top    -signal p_clk_async_reset_i  -type sig   -location {center {x 1500.000 y 2895.000}} -bondpad {center {x 1500.000 y 2850.161}}
add_pad -edge top    -signal p_clk_o              -type sig   -location {center {x 1395.000 y 2895.000}} -bondpad {center {x 1395.000 y 2850.161}}
add_pad -edge top    -signal p_clk_C_i            -type sig   -location {center {x 1325.000 y 2895.000}} -bondpad {center {x 1325.000 y 2850.161}}
add_pad -edge top    -signal p_clk_B_i            -type sig   -location {center {x 1255.000 y 2895.000}} -bondpad {center {x 1255.000 y 2850.161}}
add_pad -edge top    -signal p_clk_A_i            -type sig   -location {center {x 1185.000 y 2895.000}} -bondpad {center {x 1185.000 y 2850.161}}
add_pad -edge top    -signal p_co_8_i             -type sig   -location {center {x 1115.000 y 2895.000}} -bondpad {center {x 1115.000 y 2850.161}}
add_pad -edge top    -signal p_co_7_i             -type sig   -location {center {x 1045.000 y 2895.000}} -bondpad {center {x 1045.000 y 2850.161}}
add_pad -edge top    -signal p_co_6_i             -type sig   -location {center {x  940.000 y 2895.000}} -bondpad {center {x  940.000 y 2850.161}}
add_pad -edge top    -signal p_co_5_i             -type sig   -location {center {x  870.000 y 2895.000}} -bondpad {center {x  870.000 y 2850.161}}
add_pad -edge top    -signal p_co_v_i             -type sig   -location {center {x  800.000 y 2895.000}} -bondpad {center {x  800.000 y 2850.161}}
add_pad -edge top    -signal p_co_tkn_o           -type sig   -location {center {x  730.000 y 2895.000}} -bondpad {center {x  730.000 y 2850.161}}
add_pad -edge top    -signal p_co_clk_i           -type sig   -location {center {x  660.000 y 2895.000}} -bondpad {center {x  660.000 y 2850.161}}
add_pad -edge top    -signal p_co_4_i             -type sig   -location {center {x  590.000 y 2895.000}} -bondpad {center {x  590.000 y 2850.161}}
add_pad -edge top    -signal p_co_3_i             -type sig   -location {center {x  520.000 y 2895.000}} -bondpad {center {x  520.000 y 2850.161}}
add_pad -edge top    -signal p_co_2_i             -type sig   -location {center {x  415.000 y 2895.000}} -bondpad {center {x  415.000 y 2850.161}}
add_pad -edge top    -signal p_co_1_i             -type sig   -location {center {x  345.000 y 2895.000}} -bondpad {center {x  345.000 y 2850.161}}
add_pad -edge top    -signal p_co_0_i             -type sig   -location {center {x  275.000 y 2895.000}} -bondpad {center {x  275.000 y 2850.161}}
add_pad -edge top    -inst_name u_vzz_17  -signal DVSS_0               -type vssio -location {center {x 2725.000 y 2895.000}} -bondpad {center {x 2725.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_17  -signal DVDD_0               -type vddio -location {center {x 2655.000 y 2895.000}} -bondpad {center {x 2655.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_17  -signal VSS                  -type vss   -location {center {x 2585.000 y 2895.000}} -bondpad {center {x 2585.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_17  -signal VDD                  -type vdd   -location {center {x 2515.000 y 2895.000}} -bondpad {center {x 2515.000 y 2936.707}}
add_pad -edge top    -inst_name u_vzz_18  -signal DVSS_0               -type vssio -location {center {x 2410.000 y 2895.000}} -bondpad {center {x 2410.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_18  -signal DVDD_0               -type vddio -location {center {x 2340.000 y 2895.000}} -bondpad {center {x 2340.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_18  -signal VSS                  -type vss   -location {center {x 2270.000 y 2895.000}} -bondpad {center {x 2270.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_18  -signal VDD                  -type vdd   -location {center {x 2200.000 y 2895.000}} -bondpad {center {x 2200.000 y 2936.707}}
add_pad -edge top    -inst_name u_vzz_19  -signal DVSS_0               -type vssio -location {center {x 2130.000 y 2895.000}} -bondpad {center {x 2130.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_19  -signal DVDD_0               -type vddio -location {center {x 2060.000 y 2895.000}} -bondpad {center {x 2060.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_19  -signal VSS                  -type vss   -location {center {x 1990.000 y 2895.000}} -bondpad {center {x 1990.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_19  -signal VDD                  -type vdd   -location {center {x 1920.000 y 2895.000}} -bondpad {center {x 1920.000 y 2936.707}}
add_pad -edge top    -inst_name u_vzz_20  -signal DVSS_0               -type vssio -location {center {x 1815.000 y 2895.000}} -bondpad {center {x 1815.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_20  -signal DVDD_0               -type vddio -location {center {x 1745.000 y 2895.000}} -bondpad {center {x 1745.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_20  -signal VSS                  -type vss   -location {center {x 1675.000 y 2895.000}} -bondpad {center {x 1675.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_20  -signal VDD                  -type vdd   -location {center {x 1605.000 y 2895.000}} -bondpad {center {x 1605.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_pll -signal VSS                  -type vss   -location {center {x 1535.000 y 2895.000}} -bondpad {center {x 1535.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_pll -signal VDD                  -type vdd   -location {center {x 1465.000 y 2895.000}} -bondpad {center {x 1465.000 y 2936.707}}
add_pad -edge top    -inst_name u_vzz_21  -signal DVSS_1               -type vssio -location {center {x 1360.000 y 2895.000}} -bondpad {center {x 1360.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_21  -signal DVDD_1               -type vddio -location {center {x 1290.000 y 2895.000}} -bondpad {center {x 1290.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_21  -signal VSS                  -type vss   -location {center {x 1220.000 y 2895.000}} -bondpad {center {x 1220.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_21  -signal VDD                  -type vdd   -location {center {x 1150.000 y 2895.000}} -bondpad {center {x 1150.000 y 2936.707}}
add_pad -edge top    -inst_name u_vzz_22  -signal DVSS_0               -type vssio -location {center {x 1080.000 y 2895.000}} -bondpad {center {x 1080.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_22  -signal DVDD_0               -type vddio -location {center {x  975.000 y 2895.000}} -bondpad {center {x  975.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_22  -signal VSS                  -type vss   -location {center {x  905.000 y 2895.000}} -bondpad {center {x  905.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_22  -signal VDD                  -type vdd   -location {center {x  835.000 y 2895.000}} -bondpad {center {x  835.000 y 2936.707}}
add_pad -edge top    -inst_name u_vzz_23  -signal DVSS_0               -type vssio -location {center {x  765.000 y 2895.000}} -bondpad {center {x  765.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_23  -signal DVDD_0               -type vddio -location {center {x  695.000 y 2895.000}} -bondpad {center {x  695.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_23  -signal VSS                  -type vss   -location {center {x  625.000 y 2895.000}} -bondpad {center {x  625.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_23  -signal VDD                  -type vdd   -location {center {x  555.000 y 2895.000}} -bondpad {center {x  555.000 y 2936.707}}
add_pad -edge top    -inst_name u_vzz_24  -signal DVSS_0               -type vssio -location {center {x  450.000 y 2895.000}} -bondpad {center {x  450.000 y 2936.707}}
add_pad -edge top    -inst_name u_v18_24  -signal DVDD_0               -type vddio -location {center {x  380.000 y 2895.000}} -bondpad {center {x  380.000 y 2936.707}}
add_pad -edge top    -inst_name u_vss_24  -signal VSS                  -type vss   -location {center {x  310.000 y 2895.000}} -bondpad {center {x  310.000 y 2936.707}}
add_pad -edge top    -inst_name u_vdd_24  -signal VDD                  -type vdd   -location {center {x  240.000 y 2895.000}} -bondpad {center {x  240.000 y 2936.707}}
add_pad -edge top    -inst_name u_cbrk10               -type cbk   -location {center {x 2777.500 y 2895.000}}
add_pad -edge top    -inst_name u_cbrk11               -type cbk   -location {center {x 2182.500 y 2895.000}}
add_pad -edge top    -inst_name u_brk0                 -type fbk   -location {center {x 1587.500 y 2895.000}}
add_pad -edge top    -inst_name u_brk1                 -type fbk   -location {center {x 1272.500 y 2895.000}}
add_pad -edge top    -inst_name u_cbrk12               -type cbk   -location {center {x  747.500 y 2895.000}}
add_pad -edge top    -inst_name u_cbrk13               -type cbk   -location {center {x  222.500 y 2895.000}}
add_pad -edge top    -inst_name u_pwrdet8              -type pdt   -location {center {x  485.000 y 2895.000}}
add_pad -edge top    -inst_name u_pwrdet9              -type pdt   -location {center {x 1010.000 y 2895.000}}
add_pad -edge top    -inst_name u_pwrdet10             -type pdt   -location {center {x 1430.000 y 2895.000}}
add_pad -edge top    -inst_name u_pwrdet11             -type pdt   -location {center {x 1885.000 y 2895.000}}
add_pad -edge top    -inst_name u_pwrdet12             -type pdt   -location {center {x 2480.000 y 2895.000}}
add_pad -edge left   -signal p_co2_8_o            -type sig   -location {center {x  105.000 y 2755.000}} -bondpad {center {x  63.293 y 2755.000}}
add_pad -edge left   -signal p_co2_7_o            -type sig   -location {center {x  105.000 y 2681.000}} -bondpad {center {x  63.293 y 2681.000}}
add_pad -edge left   -signal p_co2_6_o            -type sig   -location {center {x  105.000 y 2607.000}} -bondpad {center {x  63.293 y 2607.000}}
add_pad -edge left   -signal p_co2_5_o            -type sig   -location {center {x  105.000 y 2533.000}} -bondpad {center {x  63.293 y 2533.000}}
add_pad -edge left   -signal p_co2_v_o            -type sig   -location {center {x  105.000 y 2422.000}} -bondpad {center {x  63.293 y 2422.000}}
add_pad -edge left   -signal p_co2_tkn_i          -type sig   -location {center {x  105.000 y 2348.000}} -bondpad {center {x  63.293 y 2348.000}}
add_pad -edge left   -signal p_co2_clk_o          -type sig   -location {center {x  105.000 y 2274.000}} -bondpad {center {x  63.293 y 2274.000}}
add_pad -edge left   -signal p_co2_4_o            -type sig   -location {center {x  105.000 y 2200.000}} -bondpad {center {x  63.293 y 2200.000}}
add_pad -edge left   -signal p_co2_3_o            -type sig   -location {center {x  105.000 y 2128.000}} -bondpad {center {x  63.293 y 2128.000}}
add_pad -edge left   -signal p_co2_2_o            -type sig   -location {center {x  105.000 y 2054.000}} -bondpad {center {x  63.293 y 2054.000}}
add_pad -edge left   -signal p_co2_1_o            -type sig   -location {center {x  105.000 y 1980.000}} -bondpad {center {x  63.293 y 1980.000}}
add_pad -edge left   -signal p_co2_0_o            -type sig   -location {center {x  105.000 y 1906.000}} -bondpad {center {x  63.293 y 1906.000}}
add_pad -edge left   -signal p_bsg_tag_clk_o      -type sig   -location {center {x  105.000 y 1795.000}} -bondpad {center {x  63.293 y 1795.000}}
add_pad -edge left   -signal p_bsg_tag_data_o     -type sig   -location {center {x  105.000 y 1721.000}} -bondpad {center {x  63.293 y 1721.000}}
add_pad -edge left   -signal p_ddr_dq_7_io        -type sig   -location {center {x  105.000 y 1647.000}} -bondpad {center {x  63.293 y 1647.000}}
add_pad -edge left   -signal p_ddr_dq_6_io        -type sig   -location {center {x  105.000 y 1573.000}} -bondpad {center {x  63.293 y 1573.000}}
add_pad -edge left   -signal p_ddr_dq_5_io        -type sig   -location {center {x  105.000 y 1501.000}} -bondpad {center {x  63.293 y 1501.000}}
add_pad -edge left   -signal p_ddr_dq_4_io        -type sig   -location {center {x  105.000 y 1427.000}} -bondpad {center {x  63.293 y 1427.000}}
add_pad -edge left   -signal p_ddr_dq_3_io        -type sig   -location {center {x  105.000 y 1353.000}} -bondpad {center {x  63.293 y 1353.000}}
add_pad -edge left   -signal p_ddr_dq_2_io        -type sig   -location {center {x  105.000 y 1279.000}} -bondpad {center {x  63.293 y 1279.000}}
add_pad -edge left   -signal p_ddr_dq_1_io        -type sig   -location {center {x  105.000 y 1168.000}} -bondpad {center {x  63.293 y 1168.000}}
add_pad -edge left   -signal p_ddr_dq_0_io        -type sig   -location {center {x  105.000 y 1094.000}} -bondpad {center {x  63.293 y 1094.000}}
add_pad -edge left   -signal p_ddr_dm_0_o         -type sig   -location {center {x  105.000 y 1020.000}} -bondpad {center {x  63.293 y 1020.000}}
add_pad -edge left   -signal p_ddr_dqs_n_0_io     -type sig   -location {center {x  105.000 y  946.000}} -bondpad {center {x  63.293 y  946.000}}
add_pad -edge left   -signal p_ddr_dqs_p_0_io     -type sig   -location {center {x  105.000 y  874.000}} -bondpad {center {x  63.293 y  874.000}}
add_pad -edge left   -signal p_ddr_dq_15_io       -type sig   -location {center {x  105.000 y  800.000}} -bondpad {center {x  63.293 y  800.000}}
add_pad -edge left   -signal p_ddr_dq_14_io       -type sig   -location {center {x  105.000 y  726.000}} -bondpad {center {x  63.293 y  726.000}}
add_pad -edge left   -signal p_ddr_dq_13_io       -type sig   -location {center {x  105.000 y  652.000}} -bondpad {center {x  63.293 y  652.000}}
add_pad -edge left   -signal p_ddr_dq_12_io       -type sig   -location {center {x  105.000 y  578.000}} -bondpad {center {x  63.293 y  578.000}}
add_pad -edge left   -signal p_ddr_dq_11_io       -type sig   -location {center {x  105.000 y  467.000}} -bondpad {center {x  63.293 y  467.000}}
add_pad -edge left   -signal p_ddr_dq_10_io       -type sig   -location {center {x  105.000 y  393.000}} -bondpad {center {x  63.293 y  393.000}}
add_pad -edge left   -signal p_ddr_dq_9_io        -type sig   -location {center {x  105.000 y  319.000}} -bondpad {center {x  63.293 y  319.000}}
add_pad -edge left   -signal p_ddr_dq_8_io        -type sig   -location {center {x  105.000 y  245.000}} -bondpad {center {x  63.293 y  245.000}}
puts "Detect duplicate signal name (not power/ground) specified"
catch {add_pad -edge left   -signal p_ddr_dq_8_io        -type sig   -location {center {x  105.000 y  245.000}} -bondpad {center {x  63.293 y  245.000}}}
add_pad -edge left   -inst_name u_vzz_25  -signal DVSS_0               -type vssio -location {center {x  105.000 y 2718.000}} -bondpad {center {x 149.893 y 2718.000}}
add_pad -edge left   -inst_name u_v18_25  -signal DVDD_0               -type vddio -location {center {x  105.000 y 2644.000}} -bondpad {center {x 149.893 y 2644.000}}
add_pad -edge left   -inst_name u_vss_25  -signal VSS                  -type vss   -location {center {x  105.000 y 2570.000}} -bondpad {center {x 149.893 y 2570.000}}
add_pad -edge left   -inst_name u_vdd_25  -signal VDD                  -type vdd   -location {center {x  105.000 y 2496.000}} -bondpad {center {x 149.893 y 2496.000}}
add_pad -edge left   -inst_name u_vzz_26  -signal DVSS_0               -type vssio -location {center {x  105.000 y 2385.000}} -bondpad {center {x 149.893 y 2385.000}}
add_pad -edge left   -inst_name u_v18_26  -signal DVDD_0               -type vddio -location {center {x  105.000 y 2311.000}} -bondpad {center {x 149.893 y 2311.000}}
add_pad -edge left   -inst_name u_vss_26  -signal VSS                  -type vss   -location {center {x  105.000 y 2237.000}} -bondpad {center {x 149.893 y 2237.000}}
add_pad -edge left   -inst_name u_vdd_26  -signal VDD                  -type vdd   -location {center {x  105.000 y 2163.000}} -bondpad {center {x 149.893 y 2163.000}}
add_pad -edge left   -inst_name u_vzz_27  -signal DVSS_0               -type vssio -location {center {x  105.000 y 2091.000}} -bondpad {center {x 149.893 y 2091.000}}
add_pad -edge left   -inst_name u_v18_27  -signal DVDD_0               -type vddio -location {center {x  105.000 y 2017.000}} -bondpad {center {x 149.893 y 2017.000}}
add_pad -edge left   -inst_name u_vss_27  -signal VSS                  -type vss   -location {center {x  105.000 y 1943.000}} -bondpad {center {x 149.893 y 1943.000}}
add_pad -edge left   -inst_name u_vdd_27  -signal VDD                  -type vdd   -location {center {x  105.000 y 1869.000}} -bondpad {center {x 149.893 y 1869.000}}
add_pad -edge left   -inst_name u_vzz_28  -signal DVSS_0               -type vssio -location {center {x  105.000 y 1758.000}} -bondpad {center {x 149.893 y 1758.000}}
add_pad -edge left   -inst_name u_v18_28  -signal DVDD_0               -type vddio -location {center {x  105.000 y 1684.000}} -bondpad {center {x 149.893 y 1684.000}}
add_pad -edge left   -inst_name u_vss_28  -signal VSS                  -type vss   -location {center {x  105.000 y 1610.000}} -bondpad {center {x 149.893 y 1610.000}}
add_pad -edge left   -inst_name u_vdd_28  -signal VDD                  -type vdd   -location {center {x  105.000 y 1536.000}} -bondpad {center {x 149.893 y 1536.000}}
add_pad -edge left   -inst_name u_vzz_29  -signal DVSS_0               -type vssio -location {center {x  105.000 y 1464.000}} -bondpad {center {x 149.893 y 1464.000}}
add_pad -edge left   -inst_name u_v18_29  -signal DVDD_0               -type vddio -location {center {x  105.000 y 1390.000}} -bondpad {center {x 149.893 y 1390.000}}
add_pad -edge left   -inst_name u_vss_29  -signal VSS                  -type vssio -location {center {x  105.000 y 1316.000}} -bondpad {center {x 149.893 y 1316.000}}
add_pad -edge left   -inst_name u_vdd_29  -signal VDD                  -type vddio -location {center {x  105.000 y 1242.000}} -bondpad {center {x 149.893 y 1242.000}}
add_pad -edge left   -inst_name u_vzz_30  -signal DVSS_0               -type vss   -location {center {x  105.000 y 1131.000}} -bondpad {center {x 149.893 y 1131.000}}
add_pad -edge left   -inst_name u_v18_30  -signal DVDD_0               -type vdd   -location {center {x  105.000 y 1057.000}} -bondpad {center {x 149.893 y 1057.000}}
add_pad -edge left   -inst_name u_vss_30  -signal VSS                  -type vssio -location {center {x  105.000 y  983.000}} -bondpad {center {x 149.893 y  983.000}}
add_pad -edge left   -inst_name u_vdd_30  -signal VDD                  -type vddio -location {center {x  105.000 y  909.000}} -bondpad {center {x 149.893 y  909.000}}
add_pad -edge left   -inst_name u_vzz_31  -signal DVSS_0               -type vss   -location {center {x  105.000 y  837.000}} -bondpad {center {x 149.893 y  837.000}}
add_pad -edge left   -inst_name u_v18_31  -signal DVDD_0               -type vdd   -location {center {x  105.000 y  763.000}} -bondpad {center {x 149.893 y  763.000}}
add_pad -edge left   -inst_name u_vss_31  -signal VSS                  -type vssio -location {center {x  105.000 y  689.000}} -bondpad {center {x 149.893 y  689.000}}
add_pad -edge left   -inst_name u_vdd_31  -signal VDD                  -type vddio -location {center {x  105.000 y  615.000}} -bondpad {center {x 149.893 y  615.000}}
add_pad -edge left   -inst_name u_vzz_32  -signal DVSS_0               -type vss   -location {center {x  105.000 y  504.000}} -bondpad {center {x 149.893 y  504.000}}
add_pad -edge left   -inst_name u_v18_32  -signal DVDD_0               -type vdd   -location {center {x  105.000 y  430.000}} -bondpad {center {x 149.893 y  430.000}}
add_pad -edge left   -inst_name u_vss_32  -signal VSS                  -type vssio -location {center {x  105.000 y  356.000}} -bondpad {center {x 149.893 y  356.000}}
add_pad -edge left   -inst_name u_vdd_32  -signal VDD                  -type vddio -location {center {x  105.000 y  282.000}} -bondpad {center {x 149.893 y  282.000}}
add_pad -edge left   -inst_name u_cbrk14               -type cbk   -location {center {x  105.000 y 2772.500}}
puts "Detect duplicate inst_name specified"
catch {add_pad -edge left   -inst_name u_cbrk14               -type cbk   -location {center {x  105.000 y 2772.500}}}
add_pad -edge left   -inst_name u_cbrk15               -type cbk   -location {center {x  105.000 y 2145.500}}
add_pad -edge left   -inst_name u_cbrk16               -type cbk   -location {center {x  105.000 y 1518.500}}
add_pad -edge left   -inst_name u_cbrk17               -type cbk   -location {center {x  105.000 y  891.500}}
add_pad -edge left   -inst_name u_cbrk18               -type cbk   -location {center {x  105.000 y  227.500}}
add_pad -edge left   -inst_name u_pwrdet13             -type pdt   -location {center {x  105.000 y 2459.000}}
add_pad -edge left   -inst_name u_pwrdet14             -type pdt   -location {center {x  105.000 y 1832.000}}
add_pad -edge left   -inst_name u_pwrdet15             -type pdt   -location {center {x  105.000 y 1205.000}}
add_pad -edge left   -inst_name u_pwrdet16             -type pdt   -location {center {x  105.000 y  541.000}}

initialize_floorplan \
  -die_area  {0 0 3000.000 3000.000} \
  -core_area {180.012 180.096 2819.964 2819.712} \
  -site      FreePDK45_38x28_10R_NP_162NW_34O
make_tracks

if {[catch {initialize_padring} msg]} {
  puts $errorInfo
  puts $msg
  return
}

set def_file [make_result_file "tcl_interface.def"]

write_def $def_file
diff_files $def_file "tcl_interface.defok"
