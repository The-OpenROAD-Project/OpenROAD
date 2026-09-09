# IR drop analysis on a real backside-power-delivery (BSPDN) design.
#
# GCD placed on GT2N, an open 2nm nanosheet PDK whose power comes in from
# the wafer backside: standard cell vdd / vss pins are on BPR (buried power
# rail) and pdngen builds the grid entirely on backside metal
# (BPR followpins -> BV0 -> BM1 -> BV1 -> BM2), with the vdd / vss bterms
# on BM2. The PDK lives in test/gt2n_data (shared, symlinked in as
# gt2n_data); the placed design is in gt2n_gcd_data. See the README in
# each for provenance.
#
# This complements check_power_grid_backside, which uses a synthetic
# single-backside-layer LEF and only checks connectivity. Here the whole
# stack is real, so it covers the parts a hand-built case cannot:
#   * ITerms whose base node resolves onto a backside layer for every
#     instance in the design, not just a couple of modeled bridge cells,
#   * backside power taps (gt2_6t_tapbspdn_w31_svt),
#   * bterm-sourced solving on a backside layer (-source_type BUMPS),
#   * a numeric IR drop solve over backside metal, which pins down the
#     resistance map for backside layers and vias.
source helpers.tcl

read_liberty gt2n_data/gt2_6t_w31_svt_tt_0p7v25c.lib.gz
read_lef gt2n_data/gt2_tech.lef
read_lef gt2n_data/gt2_6t_w31_svt.lef
read_def gt2n_gcd_data/gcd.def
read_sdc gt2n_gcd_data/gcd.sdc

source gt2n_data/setRC.tcl

analyze_power_grid -net vdd -source_type BUMPS
