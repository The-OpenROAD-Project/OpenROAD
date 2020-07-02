# Antenna Rule Checker

Check antenna violations based on an input LEF and DEF file, a report
will be generated to indicate violated nets. 4 APIs are provided as an interface with FastRoute to preemptively fix antenna violation as well as used for the diode insertion flow:

## Antenna check Commands

---

 - `load_antenna_rules`: import antenna rules to ARC, must be called before other commands
 - `check_antennas`: check antenna violations on all nets and generate a reportt

## Antenna fix Commands

---

### Tcl commands

 - `check_net_violation`: check if an input net is violated, return 1 if the net is violated
   - -net_name: set the net name for checking
 - `get_met_avail_length`: print the available metal length that can be added to a net, while keeping other layers unchange and PAR values meet the ratio
   - -net_name: set the net name for calculation
   - -routing_level: set the layer whose rest metal length will be returned

### C++ commands

 - `PAR_max_wire_length(dbNet * net, int routing_level)`
   This function returns the max wire length allowed to add for a given net, in a
   specific layer.
