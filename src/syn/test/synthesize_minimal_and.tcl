# Minimal reproducer for a SIGSEGV in syn::map_combinationals (the post-ABC
# mapper introduced in #10473). Any design with at least one AND node in
# the post-ABC AIG crashes; a single 2-input AND is the smallest input
# that triggers it.
#
# Expected behavior once fixed: synthesize completes and "OK" is printed.
# Current behavior: SIGSEGV inside syn::map_combinationals immediately
# after the "[INFO SYN-0023] abc_roundtrip: after ABC: 1 ANDs ..." log line.
#
# Paths are workspace-root-relative; the wrapper sh_test cd's into the
# Bazel runfiles workspace before invoking openroad.

read_lef test/asap7/asap7_tech_1x_201209.lef
read_lef test/asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_liberty test/asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty test/asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty test/asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty test/asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty test/asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib

sv_elaborate -D SYNTHESIS --top synthesize_minimal_and --std=1364-2005 \
    src/syn/test/synthesize_minimal_and.sv
syn::stats
synthesize

puts "OK"
