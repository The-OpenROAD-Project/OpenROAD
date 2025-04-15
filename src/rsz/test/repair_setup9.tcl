# rebuffering on top-level input port
source helpers.tcl
read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_setup9.def

create_clock -period 1 clk
set_input_delay 0.7 -clock clk data

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

# repair_timing ignores top-level input ports; until that's fixed
# invoke rebuffering through the debug command below
rsz::rebuffer_net [[sta::top_instance] find_pin data]
