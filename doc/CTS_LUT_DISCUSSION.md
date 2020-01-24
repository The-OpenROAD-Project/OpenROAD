January 2020
Tom Spyrou in consultation with James Cherry

In looking at the current LUT Characterization for CTS, there are about 4K lines in the look up table file. In the past it seems that generating these took a long time which does not make sense. OpenSTA should be able to run those 4K patterns in seconds.

We have been looking at extending the TCL API of OpenSTA to return liberty table information. This heads down a slippery slope of complexity since the tables are not regularly sized in all libraries. It is also possible to load multiple liberty files in a single run which are characterized differently. There may be cells from different vendors used in a single design for example. Taken to its logical extreme the CTS lookup tables could end up being a superset of Liberty and LEF.

Rather than making the look up table characterization more complex and continuing to develop it in TCL, we should make it simpler, rely on the sta and delay calculator C++ APIs more, and characterize the look up tables needed for the dynamic programming on the fly at tool startup. 

The current architecture is using 1980’s style software architecture assumptions by not relying on the ability to call OpenSTA in an integrated app during CTS but rather trying to pull out everything that might ever be needed from OpenSTA up front. In OpenROAD this is not necessary. The CTS application is nicely integrated into OpenROAD now.

In order to demonstrate how fast OpenSTA can run, we can make a prototype in TCL that generates a netlist which contains all 4K patterns in disconnected parallel pieces of logic. The instances can be named such that each pattern can be found by looking for the correct instance name or port names, subtract the arrival times between them, and storing the delay information. If parasitics information is needed, all 4K patterns can be put into one SPEF file. This container design will have about 8K registers and some multiple of that number of buffers. OpenSTA will run this design in seconds and only a single call to updateTiming will be needed to get all of the arrival times up to date for all of the patterns.

I think we should question the use of registers in the patterns and instead of registers tie the buffer chain inputs to input ports and the buffer chain outputs to output ports. This will make the netlist simpler and avoid the transition setting on internal nodes issue we have seen in the past. All inputs and outputs can be constrained with a simple SDC which creates a virtual clock and sets all of the input arrival and output required times to 0. The input transition can be set via the SDC that creates the clock and arrival times. If it is necessary to sweep input transition times, they can be reset and timing re-updated or exact copies of the patterns can be made with different transition times set. 

create_clock -period 10 -name virtual

set_input_delay -clock virtual 0 [all_inputs]

set_output_delay -clock virtual [all_outputs]

set_input_transition 

on each input as needed, vary, rerun report_timing
output load by set_load


We can create a new tcl command in openroad.i to create a second opensta instance, load it with the template netlist, run independent of opendb and openroad, and save the LUT in a member variable of CTS.
