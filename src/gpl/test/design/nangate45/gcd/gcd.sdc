###################################################################

# Created by write_sdc on Mon Jun 17 07:25:58 2019

###################################################################
set sdc_version 2.0

set_units -time ns -resistance kOhm -capacitance pF -voltage V -current mA
create_clock [get_ports clk]  -name core_clock  -period 10  -waveform {0 5}
