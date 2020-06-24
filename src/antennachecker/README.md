# Antenna Rule Checker

Check antenna violations based on an input LEF and DEF file, a report
will be generated to indicate violated nets. 4 APIs are provided as an interface with FastRoute to preemptively fix antenna violation as well as used for the diode insertion flow:

## Antenna check Commands
 - `check_antenna`: check antenna violations on all nets and generate a report 

## Antenna fix Commands (not fully tested)
### Tcl APIs
 - `checkViolation`: check if an input net is violated
It is used in the diodes insertion tcl as: [antennachecker::checkViolation $net]

 - `getViolationITerm`: return an ITerm on an input violated net

### C++ APIs
 - `PAR_max_wire_length(dbNet * net, int routing_level)`;
     This function returns the max wirelength allowed for a given net, in a
     specific layer.

 
