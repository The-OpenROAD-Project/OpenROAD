# Antenna Rule Checker

This tool checks antenna violations based on an input LEF and DEF file, a report
will be generated to indicate violated nets. 4 APIs are provided as an interface with FastRoute to preemptively fix antenna violation as well as used for the diode insertion flow:

## Antenna check Commands

---

 - `load_antenna_rules`: import antenna rules to ARC, must be called before other commands
 - `check_antennas`: check antenna violations on all nets and generate a report
   - -path: specify the path to save the report
   - -simple_report: provides a summary of the violated nets 
## Antenna fix Commands

---

### Tcl commands

 - `check_net_violation`: check if an input net is violated, return 1 if the net is violated
   - -net_name: set the net name for checking
 - `find_max_wire_length`: print the longest wire in the design

### C++ commands

 - `PAR_max_wire_length(dbNet * net, int layer)`

   - dbNet * net: target net
   - layer: target layer of the given net will be calculated

   This function returns the max wire length allowed to add for a given net in a selected layer.

 - `get_net_antenna_violations( dbNet * net, std::string antenna_cell_name, std::string cell_pin )`

   - dbNet * net: target net
   - std::string antenna_cell_name: name of the antenna cell in the library, default is an empty string
   - std::string cell_pin: the pin name of the antenna cell that will be connected to nets, default is an empty string

   This function checks if the target net has antenna violations. The return value contains the violated ITerms at each layer of the net, the required antenna cells will be calculated if arguments **antenna_cell_name** and **cell_pin** are given.
