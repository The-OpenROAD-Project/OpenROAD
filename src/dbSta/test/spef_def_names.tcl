# Regression for SpefReader name-escape lookups when the netlist comes from
# DEF rather than verilog. The DEF stores instance/net names verbatim with
# literal '/' (post-CTS-style flat names) and '[]' (bus bits); the SPEF
# follows the SPEF grammar and decorates those characters with backslash
# escapes ('\/', '\[', '\]'). Without the SpefReader fallbacks, lookups for
# the slash- and bracket-bearing names would all miss because:
#   * Network::findInstanceRelative splits at unescaped '/' -> hierarchy walk
#     finds no parent matching the pre-'/' segment, and
#   * dbNetwork's literal block_->find* expects the unescaped form actually
#     stored by the DEF reader.
# Pass criterion: read_spef completes with no STA-1648/1650/1651 warnings.
source "helpers.tcl"
read_lef example1.lef
read_liberty example1_slow.lib
read_def spef_def_names.def
read_spef spef_def_names.spef
puts "spef_def_names: read_spef done"
