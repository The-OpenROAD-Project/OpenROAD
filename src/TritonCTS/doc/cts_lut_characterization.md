January 2021

CTS LUT characterization has been removed from the system. Characterization is now done automatically on the fly
during clock tree synthesis.
This uses technology parameters from LEF metal layer specified by "set_wire_rc -clock" and
buffer cap/slew values from liberty file. Characterization uses OpenSTA interface to generate
timing information required to complete characterization table required during CTS clock building process.