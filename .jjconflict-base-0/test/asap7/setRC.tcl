# correlation result (aes, cva6, ibex, riscv32i)
# M1 capacitance fixed up from -4.8e-02 to 1e-10 as a minuscule positive value
set_layer_rc -layer M1 -resistance 7.04175E-02 -capacitance 1e-10
set_layer_rc -layer M2 -resistance 4.62311E-02 -capacitance 1.84542E-01
set_layer_rc -layer M3 -resistance 3.63251E-02 -capacitance 1.53955E-01
set_layer_rc -layer M4 -resistance 2.03083E-02 -capacitance 1.89434E-01
set_layer_rc -layer M5 -resistance 1.93005E-02 -capacitance 1.71593E-01
set_layer_rc -layer M6 -resistance 1.18619E-02 -capacitance 1.76146E-01
set_layer_rc -layer M7 -resistance 1.25311E-02 -capacitance 1.47030E-01
set_wire_rc -signal -resistance 3.23151E-02 -capacitance 1.73323E-01
set_wire_rc -clock -resistance 5.13971E-02 -capacitance 1.44549E-01

set_layer_rc -via V1 -resistance 1.72E-02
set_layer_rc -via V2 -resistance 1.72E-02
set_layer_rc -via V3 -resistance 1.72E-02
set_layer_rc -via V4 -resistance 1.18E-02
set_layer_rc -via V5 -resistance 1.18E-02
set_layer_rc -via V6 -resistance 8.20E-03
set_layer_rc -via V7 -resistance 8.20E-03
set_layer_rc -via V8 -resistance 6.30E-03
