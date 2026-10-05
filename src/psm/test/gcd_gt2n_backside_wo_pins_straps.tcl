# Source-layer naming on the STRAPS path of a backside-power design.
#
# Same GT2N (backside power delivery) GCD as gcd_gt2n_backside_wo_pins, but
# driving the strap generator instead of the bump generator. gcd.def carries
# no vdd/vss bterms, so generateSourceNodesFromBTerms comes back empty and
# the STRAPS fallback runs, which places its sources on IRNetwork::getTopLayer().
#
# On a BSPDN stack the LEF declares the backside outermost-first
# (BRDL, BM4, BM3, BM2, BM1, BPR, M0 ... M7), so getTopLayer() has to pick the
# *first* layer holding nodes rather than the last. The PSM-0072 line below
# names that layer, so a regression of getTopLayer() fails here on a visibly
# wrong layer name instead of only on the IR numbers.
#
# The second layer PSM-0072 names is the connect layer, the one immediately
# outside the source layer, whose pitch and width set the strap pattern. On a
# backside stack the levels grow toward the substrate, so that layer is at
# routing level - 1, and this test covers that side of the choice: the layer
# named here must be further *out* than the source layer, never BM1 or BPR.
source helpers.tcl

read_liberty gt2n_data/gt2_6t_w31_svt_tt_0p7v25c.lib.gz
read_lef gt2n_data/gt2_tech.lef
read_lef gt2n_data/gt2_6t_w31_svt.lef
read_def gt2n_gcd_data/gcd.def
read_sdc gt2n_gcd_data/gcd.sdc

source gt2n_data/setRC.tcl

analyze_power_grid -net vdd -source_type STRAPS
