# test for power switch, which is not implemented
from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130_fd_sc_hd_merged.lef")
tech.readLef("sky130_power_switch/power_switch.lef")

design = Design(tech)
design.readDef("sky130_power_switch/floorplan.def")

pdn_aux.add_global_connection(design, net_name="VDD", power=True, pin_pattern="^VDDG$")
pdn_aux.add_global_connection(design, net_name="VDD_SW", power=True, pin_pattern="^VPB$")
pdn_aux.add_global_connection(design, net_name="VDD_SW", pin_pattern="^VPWR$")
pdn_aux.add_global_connection(design, net_name="VSS", power=True, pin_pattern="^VGND$")
pdn_aux.add_global_connection(design, net_name="VSS", power=True, pin_pattern="^VNB$")

pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS", switched_power_name="VDD_SW")
pdn_aux.define_power_switch_cell(design, name="POWER_SWITCH", control="SLEEP", acknowledge="SLEEP_OUT",
                                 power_switchable="VPWR", power="VDDG", ground="VGND")
pdn_aux.define_pdn_grid_real(design, name="Core", power_switch_cell="POWER_SWITCH", 
                             power_control="nPWRUP", power_control_network="DAISY")

pdn_aux.add_pdn_stripe(design, layer="met1", width=0.48, offset=0, followpins=True)
pdn_aux.add_pdn_stripe(design, layer="met4", width=1.600, pitch=27.140, offset=13.570)
pdn_aux.add_pdn_stripe(design, layer="met5", width=1.600, pitch=27.200, offset=13.600)

pdn_aux.add_pdn_connect(design, layers=["met1", "met4"])
pdn_aux.add_pdn_connect(design, layers=["met2", "met4"])
pdn_aux.add_pdn_connect(design, layers=["met4", "met5"])

design.getPdnGen().report()

# source "helpers.tcl"

# read_lef 
# read_lef sky130hd/sky130_fd_sc_hd_merged.lef 
# read_lef sky130_power_switch/power_switch.lef 

# read_def 

# add_global_connection -net VDD -power -pin_pattern "^VDDG$"
# add_global_connection -net VDD_SW -power -pin_pattern "^VPB$"
# add_global_connection -net VDD_SW -pin_pattern "^VPWR$"
# add_global_connection -net VSS -power -pin_pattern "^VGND$"
# add_global_connection -net VSS -power -pin_pattern "^VNB$"

# set_voltage_domain -power VDD -ground VSS -switched_power VDD_SW
# define_power_switch_cell -name POWER_SWITCH -control SLEEP -acknowledge SLEEP_OUT -power_switchable VPWR -power VDDG -ground VGND
# define_pdn_grid -name "Core" -power_switch_cell POWER_SWITCH -power_control nPWRUP -power_control_network DAISY

# add_pdn_connect -layers {met1 met4} 
# add_pdn_connect -layers {met2 met4} 
# add_pdn_connect -layers {met4 met5}

# pdngen -report_only
