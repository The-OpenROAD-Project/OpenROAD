# GT2N typical-corner layer RC.
#
# Values match lambdapdk's gt2n PDK definition (the "typical" pexcorner),
# which the SiliconCompiler / ORFS flows apply via set_layer_rc.  Resistance
# is ohm/um, capacitance pF/um.  The GT2N tech LEF carries no RESISTANCE /
# CAPACITANCE properties, so PSM needs these to build its resistance map.
#
# Only the layers the backside grid actually uses (BPR, BM1, BM2 and the
# BV0 / BV1 cuts between them) matter for this test; the frontside stack is
# included so the file stays a faithful copy of the PDK table.

# Backside routing (BPR = buried power rail)
set_layer_rc -layer BPR -resistance 24.31 -capacitance 7.793e-5
set_layer_rc -layer BM1 -resistance 7.48 -capacitance 1.535e-4
set_layer_rc -layer BM2 -resistance 7.48 -capacitance 1.051e-4
set_layer_rc -layer BM3 -resistance 0.64 -capacitance 1.205e-4
set_layer_rc -layer BM4 -resistance 0.64 -capacitance 8.666e-5
set_layer_rc -layer BRDL -resistance 0.01 -capacitance 1.006e-4

# Backside vias
set_layer_rc -via BV0 -resistance 25.10
set_layer_rc -via BV1 -resistance 6.08
set_layer_rc -via BV2 -resistance 6.08
set_layer_rc -via BV3 -resistance 0.95
set_layer_rc -via BV4 -resistance 0.15

# Frontside routing
set_layer_rc -layer M0 -resistance 621.75 -capacitance 1.200e-4
set_layer_rc -layer M1 -resistance 437.50 -capacitance 1.023e-4
set_layer_rc -layer M2 -resistance 621.75 -capacitance 9.980e-5
set_layer_rc -layer M3 -resistance 437.50 -capacitance 1.023e-4
set_layer_rc -layer M4 -resistance 166.95 -capacitance 1.088e-4

# Frontside vias
set_layer_rc -via V0 -resistance 54.99
set_layer_rc -via V1 -resistance 54.99
set_layer_rc -via V2 -resistance 54.99
set_layer_rc -via V3 -resistance 45.78
