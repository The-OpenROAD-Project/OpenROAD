# correlateRC.py gcd,ibex,aes,jpeg,chameleon,riscv32i,chameleon_hier
# cap units pf/um
set_layer_rc -layer Metal1 -capacitance 3.49E-05 -resistance 0.135e-03
set_layer_rc -layer Metal2 -capacitance 1.81E-05 -resistance 0.103e-03
set_layer_rc -layer Metal3 -capacitance 2.14962E-04 -resistance 0.103e-03
set_layer_rc -layer Metal4 -capacitance 1.48128E-04 -resistance 0.103e-03
set_layer_rc -layer Metal5 -capacitance 1.54087E-04 -resistance 0.103e-03
set_layer_rc -layer TopMetal1 -capacitance 1.54087E-04 -resistance 0.021e-03
set_layer_rc -layer TopMetal2 -capacitance 1.54087E-04 -resistance 0.0145e-03
# end correlate

set_layer_rc -via Via1 -resistance 2.0E-3
set_layer_rc -via Via2 -resistance 2.0E-3
set_layer_rc -via Via3 -resistance 2.0E-3
set_layer_rc -via Via4 -resistance 2.0E-3
set_layer_rc -via TopVia1 -resistance 0.4E-3
set_layer_rc -via TopVia2 -resistance 0.22E-3

set_wire_rc -signal -layer Metal2
set_wire_rc -clock -layer Metal5
