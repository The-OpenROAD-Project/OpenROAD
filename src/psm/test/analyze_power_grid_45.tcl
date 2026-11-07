# Check that a 45 degree wire has the resistance of its own length and width
# rather than of its bounding box: the same load fed through a straight and
# a 45 degree wire of the same length and width sees the same IR drop.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def analyze_power_grid_45.def
read_liberty Nangate45/Nangate45_typ.lib

set_pdnsim_net_voltage -net VDD_STRAIGHT -voltage 1.1
set_pdnsim_net_voltage -net VDD_45 -voltage 1.1
set_pdnsim_inst_power -inst u1 -power 1e-3
set_pdnsim_inst_power -inst u2 -power 1e-3

check_power_grid -net VDD_STRAIGHT -dont_require_terminals
analyze_power_grid -vsrc analyze_power_grid_45.straight.loc -net VDD_STRAIGHT

check_power_grid -net VDD_45 -dont_require_terminals
analyze_power_grid -vsrc analyze_power_grid_45.45.loc -net VDD_45
