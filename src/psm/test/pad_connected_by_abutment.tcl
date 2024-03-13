source "helpers.tcl"

read_lef ihp-sg13g2_data/sg13g2_tech.lef
read_lef ihp-sg13g2_data/sg13g2_stdcell.lef
read_lef ihp-sg13g2_data/IOLib.lef

read_def pad_connected_by_abutment.def

read_liberty ihp-sg13g2_data/sg13g2_stdcell_typ_1p20V_25C.lib
read_liberty ihp-sg13g2_data/IOLib_dummy.lib

######## Constraints
set input_ports [get_ports {\
    p_write_enable \
    p_din \
    p_clk \
    p_reset
}]

set output_ports [get_ports {\
    p_dout
}]

set_driving_cell -lib_cell IOPadInOut4mA -pin pad $input_ports
set_driving_cell -lib_cell IOPadInOut4mA -pin pad $output_ports

create_clock [get_pins u_clk/p2c] -name p_clk -period 30 -waveform {0 15}

set_input_delay -clock p_clk -max 11.25 $input_ports
set_input_delay -clock p_clk -min 6 $input_ports

set_output_delay -clock p_clk -max 11.25 $output_ports
set_output_delay -clock p_clk -min 24 $output_ports

source ihp-sg13g2_data/setRC.tcl

analyze_power_grid -net VDD
