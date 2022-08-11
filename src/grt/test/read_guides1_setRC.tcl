# correlateRC.py aes,ethmac,gcd,ibex,jpeg,sha3,uart
# Liberty units are fF,kOhm
set_layer_rc -layer M2 -resistance 4.44843E-02 -capacitance 8.08251E-02
set_layer_rc -layer M3 -resistance 2.56040E-02 -capacitance 1.26829E-01
set_layer_rc -layer M4 -resistance 1.63749E-02 -capacitance 1.38907E-01
set_layer_rc -layer M5 -resistance 1.36984E-02 -capacitance 1.16547E-01
set_layer_rc -layer M6 -resistance 8.88899E-03 -capacitance 1.24308E-01
set_layer_rc -layer M7 -resistance 1.00718E-02 -capacitance 1.16830E-01
# Designs only use M2-M7, so we don't have correlation data for these layers
set_layer_rc -layer M1 -capacitance 1.1368e-01 -resistance 1.3889e-01
set_layer_rc -layer M8 -capacitance 1.1822e-01 -resistance 7.4310e-03
set_layer_rc -layer M9 -capacitance 1.3497e-01 -resistance 6.8740e-03

set_layer_rc -via V1 -resistance 1.00E-02
set_layer_rc -via V2 -resistance 1.00E-02
set_layer_rc -via V3 -resistance 1.00E-02
set_layer_rc -via V4 -resistance 1.00E-02
set_layer_rc -via V5 -resistance 1.00E-02
set_layer_rc -via V6 -resistance 1.00E-02
set_layer_rc -via V7 -resistance 1.00E-02
set_layer_rc -via V8 -resistance 1.00E-02
set_layer_rc -via V9 -resistance 1.00E-02

set_wire_rc -resistance 2.07476E-02 -capacitance 1.24551E-01
