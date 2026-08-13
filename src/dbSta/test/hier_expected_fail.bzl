# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Known hierarchy conformance failures, as XFAIL rather than DISABLED_.

A disabled test never runs, so the day the bug is fixed nothing tells us and the
case sits switched off. An entry here instead inverts the expectation: the suite
goes red with an actionable message if the case starts passing (XPASS), so
coverage of the bug is never silently lost.

Entries are grouped by failure mode -- one xfail() per (path, symptom), listing
every netlist that fails that way -- so the corpus reads as a list of defects
rather than a list of rows. Adding a case to a known defect is one line.

Every entry should cite an OpenROAD issue number, so "expected to fail" never
becomes a shrug. File the issue first; do not fix the bug here. The campaign that
produced these has issues pending, hence the TBD placeholders.

src/dbSta/test/BUILD renders these into the manifests the two suites read.
"""

_PATHS = ["hier", "flat"]

# The aspects TestHierStructural compares. A key includes the check, unlike the
# conformance manifest: with one key per (netlist, path) the top-port-order
# defect -- which hits almost every netlist -- would XFAIL every case and mask
# the other nine aspects entirely.
_CHECKS = [
    "round_trip",
    "module_set",
    "top_ports",
    "submodule_ports",
    "declared_nets",
    "instances",
    "name_identity",
    "cell_census",
    "assigns",
    "namespace",
]

def _validate(path, issue, symptom, netlists, check = None):
    if path not in _PATHS:
        fail("unknown path '%s'; expected one of %s" % (path, _PATHS))
    if check != None and check not in _CHECKS:
        fail("unknown check '%s'; expected one of %s" % (check, _CHECKS))
    if not netlists:
        fail("xfail for '%s' lists no netlists" % symptom)

    # The rendered manifest is colon-separated, so a colon anywhere in a field
    # would silently truncate the entry when the test parses it back.
    for field in [path, issue, symptom] + netlists:
        if ":" in field:
            fail("':' is the manifest field separator, so it cannot appear " +
                 "in '%s'" % field)

# A netlist can only have one recorded symptom per key: the test reads the first
# matching row, so a second one would be dead text that still reads as coverage.
def _reject_duplicate(seen, key, symptom):
    if key in seen:
        fail("'%s' is listed twice, as '%s' and as '%s'; one entry wins and " %
             (key, seen[key], symptom) + "the other is never reported")
    seen[key] = symptom

def xfail(path, issue, symptom, netlists):
    """One conformance failure mode.

    Args:
      path: "hier" or "flat" -- which link mode fails. Keyed on (netlist, path)
        because the hier path can be broken while the flat path is clean, which
        is the expected shape of a finding here.
      issue: the OpenROAD issue number, or "TBD" while one is being filed.
      symptom: what the LEC reported, short enough to read in a failure message.
      netlists: the netlists that fail this way, by file name.

    Returns:
      A struct the BUILD file renders into a manifest line per netlist.
    """
    _validate(path, issue, symptom, netlists)
    return struct(
        path = path,
        issue = issue,
        symptom = symptom,
        netlists = netlists,
    )

def structural_xfail(path, check, issue, symptom, netlists):
    """One structural failure mode.

    Args:
      path: "hier" or "flat" -- which link mode fails.
      check: which comparison fails; one of _CHECKS.
      issue: the OpenROAD issue number, or "TBD" while one is being filed.
      symptom: what the structural diff reported.
      netlists: the netlists that fail this way. A netlist from the
        hier_cases/structural/ subdirectory keeps that prefix in its key
        ("structural/case.v"). A name may be a glob ('*' matches any run of
        characters), so a systemic defect that hits the whole corpus is one
        entry rather than 378 -- XPASS semantics are unaffected, since each case
        tests its own key.

    Returns:
      A struct the BUILD file renders into a manifest line per netlist.
    """
    _validate(path, issue, symptom, netlists, check)
    return struct(
        path = path,
        check = check,
        issue = issue,
        symptom = symptom,
        netlists = netlists,
    )

def conformance_manifest(entries):
    """Renders conformance entries as `netlist : path : issue : symptom` lines.

    Args:
      entries: xfail() structs.

    Returns:
      One line per (netlist, path), for write_file.
    """
    lines = []
    seen = {}
    for e in entries:
        for netlist in e.netlists:
            key = netlist + " : " + e.path
            _reject_duplicate(seen, key, e.symptom)
            lines.append(" : ".join([netlist, e.path, e.issue, e.symptom]))
    return lines

def structural_manifest(entries):
    """Renders entries as `netlist : path : check : issue : symptom` lines.

    Args:
      entries: structural_xfail() structs.

    Returns:
      One line per (netlist, path, check), for write_file.
    """
    lines = []
    seen = {}
    for e in entries:
        for netlist in e.netlists:
            key = " : ".join([netlist, e.path, e.check])
            _reject_duplicate(seen, key, e.symptom)
            lines.append(
                " : ".join([netlist, e.path, e.check, e.issue, e.symptom]),
            )
    return lines

# Failures of the LEC round trip: read_verilog -> link_design [-hier] ->
# write_verilog, proved against the input netlist. Grouped by symptom, and
# within a group sorted by name.
CONFORMANCE_EXPECTED_FAIL = [
    # The flat writer drops sub_module's `assign out_bus = in_bus[3:2];`
    # entirely, leaving sub_out_bus undriven and taking top_out_bus[1] and
    # top_out_single with it (SEC coverage 4/6). The hier path emits the assigns
    # correctly.
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "flat write drops a submodule bus-slice feedthrough assign, leaving two top outputs undriven",
        netlists = [
            "get_ports1.v",
        ],
    ),
    # The flat writer builds an instance name by joining the hierarchy path with
    # '/' and escapes the result, so the path instance `x` -> instance `y` is
    # emitted as `\x/y ` -- colliding with an instance whose name is literally
    # `\x/y `. The emitted module then declares two instances with the same
    # name, which is illegal Verilog: kepler-formal's reader rejects it outright
    # ("SNLDesign top contains already a SNLInstance named: x/y"). The hier path
    # keeps them distinct.
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "flat write emits a duplicate instance name when an escaped identifier collides with a synthesized hierarchy path",
        netlists = [
            "escaped_name_path_collision.v",
        ],
    ),
    # The hier writer hoists MOD0's internal feedthrough (`assign Z_b = Z_a;`)
    # into the parent as `assign port_a = net1;`. port_a is already driven by
    # mod0.Z_a, so the emitted netlist has two drivers on it. The flat path is
    # clean.
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "hier write adds a duplicate driver on an already-driven output port",
        netlists = [
            "TestInsertBuffer_BeforeLoads_Case33_post.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "partial -25.00%",
        netlists = [
            "getports_wholein.v",
            "gp_bitassign_top.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "partial -33.33%",
        netlists = [
            "gp_no_bus_ft.v",
            "gp_no_scalar_ft.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "partial -33.33%",
        netlists = [
            "sub_three_outs_one_driver.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "partial -50.00%",
        netlists = [
            "getports_nocell.v",
            "min_ft_one_read_only.v",
            "nameorder_wire_before.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "partial -50.00%",
        netlists = [
            "sub_two_outs_one_driver.v",
            "sub_two_outs_one_to_gate.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "partial -60.00%",
        netlists = [
            "busslice_same_in_two_outs.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "partial -66.67%",
        netlists = [
            "getports_bitassign.v",
            "getports_replica.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "partial -66.67%",
        netlists = [
            "fanout_two_subs.v",
            "sub_in_to_two_outs.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "partial -80.00%",
        netlists = [
            "overlap_rhs_sub.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - 'SNLDesign top contains already a SNLInstance named - x/y' — exact known pattern",
        netlists = [
            "bx_naming_escaped_slashcol_inst.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - 'SNLDesign top contains already a SNLInstance named - x/y/z' — depth-3 path variant",
        netlists = [
            "bx_naming_escaped_slashcol_inst3.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - duplicate 'wire \\u1/w+w ;' — escaped sub net \\w+w flattened to u1/w+w collides with top escaped ne",
        netlists = [
            "bx_naming_escaped_slashcol_escnet.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - duplicate 'wire \\x/p ;' — net synthesized for unconnected output port p of instance x collides wit",
        netlists = [
            "bx_naming_escaped_slashcol_port.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - emitted 'INV_X1 assign (...)' unescaped, 'unexpected ASSIGN_KW'",
        netlists = [
            "bx_naming_escaped_inst_kw_assign.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - emitted 'module module (' — top module keyword name unescaped",
        netlists = [
            "bx_naming_escaped_top_kw.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - emitted 'module top (input, z); input input;' unescaped — illegal",
        netlists = [
            "bx_naming_escaped_port_kw_input.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - emitted netlist illegal, 'unexpected MODULE_KW' (writer emitted 'wire module;')",
        netlists = [
            "bx_naming_escaped_net_kw_module.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - emitted netlist illegal, 'unexpected WIRE_KW' (writer emitted 'wire wire;')",
        netlists = [
            "bx_naming_escaped_net_kw_wire.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error - hier writer emitted 'INV_X1 assign (...)' inside submodule — illegal; flat proved only because the",
        netlists = [
            "bx_naming_escaped_d2_inst_kw.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error - hier writer emitted 'module module (' and 'module u1 (...)' unescaped — illegal; flat proved becau",
        netlists = [
            "bx_naming_escaped_mod_kw.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error - hier writer emitted 'output output;' and '.output(z)' unescaped — illegal; flat proved only becaus",
        netlists = [
            "bx_naming_escaped_d2_port_kw.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error - out_flat.v declares 'wire \\x/y ;' TWICE with two different drivers (top escaped net vs flattened s",
        netlists = [
            "bx_naming_escaped_slashcol_net.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error - same",
        netlists = [
            "bx_naming_escaped_inst_kw_assign.v",
            "bx_naming_escaped_port_kw_input.v",
            "bx_naming_escaped_top_kw.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error - same, 'wire module;' unescaped",
        netlists = [
            "bx_naming_escaped_net_kw_module.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error - same, 'wire wire;' unescaped",
        netlists = [
            "bx_naming_escaped_net_kw_wire.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error -A port cannot be found in INV_X1 model (out_hier.v line 22)",
        netlists = [
            "bx_collisions_uniq_clone_eq_libcell.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SEC cannot run - no aligned observed outputs remain after skipping multi-driver cones",
        netlists = [
            "bx_collisions_outport_vs_flatnet.v",
            "bx_collisions_port_in_vs_flatnet.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SEC cannot run on this design pair - Missing observed output expression for `197.0.`",
        netlists = [
            "nameorder_out_before_in.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SEC cannot run on this design pair - No aligned observed outputs remain after skipping cones with no",
        netlists = [
            "gp_full_inbus.v",
            "gp_no_topin_in_concat.v",
            "nameorder_busslice.v",
            "nameorder_deep_chain.v",
            "nameorder_h_before_i.v",
            "nameorder_minimal_repro.v",
        ],
    ),
    xfail(
        path = "hier",
        issue = "TBD",
        symptom = "tool-error -SEC cannot run on this design pair - No aligned observed outputs remain after skipping cones with no",
        netlists = [
            "sub_out_from_out.v",
            "sub_out_from_out_bus.v",
            "sub_out_from_out_deep3.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - a/b/c",
        netlists = [
            "bx_collisions_inst_deep3.v",
            "bx_collisions_synth_inst.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - m/c/d",
        netlists = [
            "bx_collisions_submodule_esc_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - x/y",
        netlists = [
            "bx_collisions_known3_esc_first.v",
            "bx_collisions_known3_inst_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - x/y/z",
        netlists = [
            "bx_collisions_inst_mixed_esc.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - x/y[0]",
        netlists = [
            "bx_collisions_escbracket_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -wire collision for net a/b/n (out_flat.v line 11)",
        netlists = [
            "bx_collisions_net_deep3.v",
            "bx_collisions_synth_net.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -wire collision for net m/c/n (out_flat.v line 11)",
        netlists = [
            "bx_collisions_subnet_esc_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -wire collision for net x/p (out_flat.v line 11)",
        netlists = [
            "bx_collisions_unconn_port_vs_esc.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -wire collision for net x/q (out_flat.v line 13)",
        netlists = [
            "bx_collisions_dff_net_vs_esc.v",
        ],
    ),
    xfail(
        path = "flat",
        issue = "TBD",
        symptom = "tool-error -wire collision for net x/y (out_flat.v line 11)",
        netlists = [
            "bx_collisions_net_vs_flatnet.v",
        ],
    ),
]

# Failures of the structural diff: the same round trip compared against the
# input for names, port order, module set, declared objects and instance
# bindings -- the fidelity aspects a LEC is blind to. Grouped by check, then by
# symptom.
STRUCTURAL_EXPECTED_FAIL = [

    # ----------------------------------------------------------------------
    # round_trip
    #
    # read_verilog/link_design refuses the input netlist, or refuses the netlist
    # it just emitted. The second form is the more serious: whatever OpenROAD
    # wrote, it cannot read back. A refused input is a finding in its own right
    # and is most of what the structural/ subdirectory holds -- those cases are
    # in no LEC suite because there is nothing for a LEC to compare.
    structural_xfail(
        path = "flat",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist (ORD-2013)",
        netlists = [
            "structural/wb_sta_reader_blackbox_bus_bit_order.v",
            "structural/wb_sta_reader_blackbox_ordered_ports.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist (STA-0171)",
        netlists = [
            "structural/wb_sta_reader_empty_specify.v",
            "structural/wb_sta_reader_tri0_net_type.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist (STA-1390)",
        netlists = [
            "structural/wb_dbsta_link_attr_impl_oper_unused.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist by throwing with no OpenROAD error code",
        netlists = [
            "structural/wb_sta_reader_const_negative_width.v",
        ],
    ),
    # Moved here from the LEC corpus 2026-08-12: 1'bx has no binary value, so a
    # LEC cannot adjudicate it by construction (the oracle refuses with "no
    # aligned observed outputs"). The structural checker sees what write_verilog
    # actually emitted for the x connection, which is what the case was written
    # to record.
    structural_xfail(
        path = "flat",
        check = "round_trip",
        issue = "TBD",
        symptom = "reader rejects an uppercase X digit (STA-0171) though lowercase x is accepted silently",
        netlists = [
            "structural/wb_sta_reader_const_upper_x_digit.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        issue = "TBD",
        symptom = "the emitted netlist cannot be read back (STA-0171)",
        netlists = [
            "bx_naming_escaped_inst_kw_assign.v",
            "bx_naming_escaped_net_kw_module.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "bx_naming_escaped_top_kw.v",
            "structural/wb_writer_wire_index_overflow.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist (ORD-2013)",
        netlists = [
            "structural/wb_sta_reader_blackbox_bus_bit_order.v",
            "structural/wb_sta_reader_blackbox_ordered_ports.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist (STA-0171)",
        netlists = [
            "structural/wb_sta_reader_empty_specify.v",
            "structural/wb_sta_reader_tri0_net_type.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist (STA-1390)",
        netlists = [
            "structural/wb_dbsta_link_attr_impl_oper_unused.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        issue = "TBD",
        symptom = "read_verilog/link_design rejects the input netlist by throwing with no OpenROAD error code",
        netlists = [
            "structural/wb_sta_reader_const_negative_width.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        issue = "TBD",
        symptom = "reader rejects an uppercase X digit (STA-0171) though lowercase x is accepted silently",
        netlists = [
            "structural/wb_sta_reader_const_upper_x_digit.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        issue = "TBD",
        symptom = "the emitted netlist cannot be read back (STA-0171)",
        netlists = [
            "bx_naming_escaped_d2_inst_kw.v",
            "bx_naming_escaped_d2_port_kw.v",
            "bx_naming_escaped_inst_kw_assign.v",
            "bx_naming_escaped_mod_kw.v",
            "bx_naming_escaped_net_kw_module.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "bx_naming_escaped_top_kw.v",
            "structural/wb_writer_wire_index_overflow.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # module_set
    #
    # The set of module definitions is not preserved. Almost all of these are
    # dbModule::makeUniqueDbModule cloning a shared module once per instance
    # (`sub` becomes `sub`, `sub_u2`), which no LEC can see because every
    # reference is rewritten consistently. This check fires on any cloning,
    # benign or not; name_identity below is what separates the two.
    structural_xfail(
        path = "hier",
        check = "module_set",
        issue = "TBD",
        symptom = "a module definition is dropped",
        netlists = [
            "structural/bx_dangling_module_never_instantiated.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "module_set",
        issue = "TBD",
        symptom = "hier uniquification clones a module per instance",
        netlists = [
            "busslice_two_insts.v",
            "bx_collisions_base_uniq2.v",
            "bx_collisions_escmod_uniq.v",
            "bx_collisions_inst_named_module_x2.v",
            "bx_collisions_module_prefix_multi.v",
            "bx_collisions_uniq_chain.v",
            "bx_collisions_uniq_clone_eq_libcell.v",
            "bx_collisions_uniq_clone_steals_top.v",
            "bx_collisions_uniq_cross_prefix.v",
            "bx_collisions_uniq_inst_named_sub1.v",
            "bx_collisions_uniq_ports_tied_diff.v",
            "bx_collisions_uniq_resolution_suffix.v",
            "bx_collisions_uniq_same_instname2.v",
            "bx_collisions_uniq_same_instname3.v",
            "bx_collisions_uniq_synth_eq_existing.v",
            "bx_collisions_uniq_theft_order_a.v",
            "bx_collisions_uniq_theft_order_z.v",
            "bx_collisions_uniq_theft_victim_below.v",
            "bx_collisions_uniq_user_mod_below_top.v",
            "bx_collisions_uniq_vs_module_collide.v",
            "bx_collisions_uniq_vs_module_collide_rev.v",
            "bx_collisions_uniq_vs_module_same.v",
            "bx_collisions_uniq_wire_named_clone.v",
            "bx_naming_plain_case_insts_d3.v",
            "escaped_hier_names.v",
            "fanout_two_subs.v",
            "four_subs_chained.v",
            "inherited/TestReadVerilog_DeepDescendantModBTermCollision.v",
            "inherited/modnet_port_alias.v",
            "structural/bx_sequential_shift8_4mods.v",
            "two_subs_chained.v",
            "uniquified_module_collision.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # top_ports
    #
    # write_verilog rewrites the top module's port list order (scalars roughly
    # alphabetical, bus ports appended). Any parent that instantiates the block
    # positionally is silently miswired. The order of a hier submodule's ports,
    # by contrast, is preserved -- hence the separate check.
    structural_xfail(
        path = "flat",
        check = "top_ports",
        issue = "TBD",
        symptom = "port list reordered",
        netlists = [
            "alias_net_on_port.v",
            "bitsel_scatter_sub.v",
            "busslice_and_gate_sub.v",
            "busslice_ascending.v",
            "busslice_deep_leaf.v",
            "busslice_leaf.v",
            "busslice_mid.v",
            "busslice_offset.v",
            "busslice_same_in_two_outs.v",
            "busslice_top.v",
            "busslice_two_insts.v",
            "bx_collisions_bracket_underscore_alias.v",
            "bx_collisions_bracket_underscore_suffix.v",
            "bx_collisions_dff_net_vs_esc.v",
            "bx_collisions_escaped_bracket_form.v",
            "bx_collisions_port_in_vs_flatnet.v",
            "bx_collisions_topport_escaped.v",
            "bx_naming_escaped_busbit_port_collide.v",
            "bx_naming_escaped_d2_busport.v",
            "bx_naming_escaped_dotpath_net.v",
            "bx_naming_escaped_instbit_lookalike.v",
            "bx_naming_escaped_portbit_collide.v",
            "bx_naming_escaped_slashcol_escnet.v",
            "bx_naming_escaped_slashcol_inst.v",
            "bx_naming_escaped_slashcol_instnet.v",
            "bx_naming_escaped_slashcol_net.v",
            "bx_naming_escaped_slashcol_port.v",
            "bx_naming_escaped_trailchar.v",
            "bx_naming_plain_case_ports_d1.v",
            "bx_naming_plain_dollar_port_d1.v",
            "bx_naming_plain_keywordish_ports_d1.v",
            "bx_naming_plain_shadow_module_dff_comb.v",
            "bx_naming_plain_underscore_ports_d1.v",
            "chain1_sub.v",
            "chain1_top.v",
            "chain2_sub.v",
            "chain2_top.v",
            "chain4_sub.v",
            "chain4_top.v",
            "chain_across_boundary.v",
            "concat_both_sub.v",
            "concat_lhs_sub.v",
            "concat_lhs_top.v",
            "concat_read_ft_wire_mixed.v",
            "concat_rhs_sub.v",
            "concatin_bus_topwire.v",
            "depth2_rename.v",
            "depth4_rename.v",
            "depth4_samename.v",
            "diamond_alias.v",
            "escaped_bus_sub.v",
            "escaped_net_top.v",
            "escaped_ports_sub.v",
            "escaped_sub_net.v",
            "fanout_two_paths.v",
            "fanout_two_subs.v",
            "feedthrough_multilevel.v",
            "four_subs_chained.v",
            "ft_bitsel_rhs_scalar_out.v",
            "ft_busslice_to_topwire.v",
            "ft_busslice_to_topwire_bitsel.v",
            "ft_fullbus_to_topwire.v",
            "ft_scalar_to_topwire.v",
            "fullbus_sub.v",
            "getports_bitassign.v",
            "getports_directports.v",
            "getports_nocell.v",
            "getports_replica.v",
            "getports_wholein.v",
            "gp_bitassign_top.v",
            "gp_full_inbus.v",
            "gp_no_bus_ft.v",
            "gp_no_scalar_ft.v",
            "gp_no_topin_in_concat.v",
            "gp_one_out_split_dest.v",
            "in_to_two_outs.v",
            "inherited/TestBufferRemoval3_feedthrough.v",
            "inherited/TestInsertBuffer_BeforeLoads_Case34_pre.v",
            "inherited/TestInsertBuffer_BusBitModNetName_pre.v",
            "inherited/TestReadVerilog_BusBitAndEscapedScalarAreDistinct.v",
            "inherited/get_ports1.v",
            "inherited/hier3.v",
            "lhs_partsel_sub.v",
            "min_ft_one_read_only.v",
            "min_ft_renamed.v",
            "min_ft_shared_inbus.v",
            "min_ft_two_reads_bitwise.v",
            "min_ft_two_reads_concat.v",
            "mined_3ddad16377_feedthrough_port_modnet.v",
            "mixed_paths_sub_and_top.v",
            "nameorder_gate_driven_control.v",
            "nameorder_input_first.v",
            "nameorder_out_before_in.v",
            "nameorder_wire_after.v",
            "out_from_out_sub_to_top.v",
            "out_readback.v",
            "overlap_rhs_sub.v",
            "slicedin_bus_topwire.v",
            "slicedin_scalar_topwire.v",
            "slicedin_split_read.v",
            "structural/bx_bus_geometry_concat_nested_in.v",
            "structural/bx_bus_geometry_inport_scalar_vec.v",
            "structural/bx_dangling_dff_ck_only.v",
            "structural/bx_port_rewiring_header_slice_port.v",
            "structural/bx_port_rewiring_width_leaf_pin_wide_net.v",
            "structural/bx_sequential_probe_sdff.v",
            "structural/bx_sequential_shift8_4mods.v",
            "structural/bx_sequential_shift8_flat.v",
            "structural/bx_sequential_shift8_nest4.v",
            "structural/bx_sequential_two_clk_two_subs.v",
            "structural/s2_portwiring_width_pad_positional_d1.v",
            "structural/wb_sta_reader_header_partselect_port.v",
            "sub_ft_out_loaded_inside.v",
            "sub_in_to_two_outs.v",
            "sub_out_alias_and_gate.v",
            "swap_slices_sub.v",
            "two_subs_chained.v",
            "twoout_ft_topwires.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "top_ports",
        issue = "TBD",
        symptom = "port list reordered and a direction or bus range changed",
        netlists = [
            "structural/wb_sta_reader_header_bitselect_port.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "top_ports",
        issue = "TBD",
        symptom = "port list reordered and a port lost",
        netlists = [
            "bx_naming_escaped_port_kw_input.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "top_ports",
        issue = "TBD",
        symptom = "port list reordered",
        netlists = [
            "alias_net_on_port.v",
            "bitsel_scatter_sub.v",
            "busslice_and_gate_sub.v",
            "busslice_ascending.v",
            "busslice_deep_leaf.v",
            "busslice_leaf.v",
            "busslice_mid.v",
            "busslice_offset.v",
            "busslice_same_in_two_outs.v",
            "busslice_top.v",
            "busslice_two_insts.v",
            "bx_collisions_bracket_underscore_alias.v",
            "bx_collisions_bracket_underscore_suffix.v",
            "bx_collisions_dff_net_vs_esc.v",
            "bx_collisions_escaped_bracket_form.v",
            "bx_collisions_port_in_vs_flatnet.v",
            "bx_collisions_topport_escaped.v",
            "bx_naming_escaped_busbit_port_collide.v",
            "bx_naming_escaped_d2_busport.v",
            "bx_naming_escaped_dotpath_net.v",
            "bx_naming_escaped_instbit_lookalike.v",
            "bx_naming_escaped_portbit_collide.v",
            "bx_naming_escaped_slashcol_escnet.v",
            "bx_naming_escaped_slashcol_inst.v",
            "bx_naming_escaped_slashcol_instnet.v",
            "bx_naming_escaped_slashcol_net.v",
            "bx_naming_escaped_slashcol_port.v",
            "bx_naming_escaped_trailchar.v",
            "bx_naming_plain_case_ports_d1.v",
            "bx_naming_plain_dollar_port_d1.v",
            "bx_naming_plain_keywordish_ports_d1.v",
            "bx_naming_plain_shadow_module_dff_comb.v",
            "bx_naming_plain_underscore_ports_d1.v",
            "chain1_sub.v",
            "chain1_top.v",
            "chain2_sub.v",
            "chain2_top.v",
            "chain4_sub.v",
            "chain4_top.v",
            "chain_across_boundary.v",
            "concat_both_sub.v",
            "concat_lhs_sub.v",
            "concat_lhs_top.v",
            "concat_read_ft_wire_mixed.v",
            "concat_rhs_sub.v",
            "concatin_bus_topwire.v",
            "depth2_rename.v",
            "depth4_rename.v",
            "depth4_samename.v",
            "diamond_alias.v",
            "escaped_bus_sub.v",
            "escaped_net_top.v",
            "escaped_ports_sub.v",
            "escaped_sub_net.v",
            "fanout_two_paths.v",
            "fanout_two_subs.v",
            "feedthrough_multilevel.v",
            "four_subs_chained.v",
            "ft_bitsel_rhs_scalar_out.v",
            "ft_busslice_to_topwire.v",
            "ft_busslice_to_topwire_bitsel.v",
            "ft_fullbus_to_topwire.v",
            "ft_scalar_to_topwire.v",
            "fullbus_sub.v",
            "getports_bitassign.v",
            "getports_directports.v",
            "getports_nocell.v",
            "getports_replica.v",
            "getports_wholein.v",
            "gp_bitassign_top.v",
            "gp_full_inbus.v",
            "gp_no_bus_ft.v",
            "gp_no_scalar_ft.v",
            "gp_no_topin_in_concat.v",
            "gp_one_out_split_dest.v",
            "in_to_two_outs.v",
            "inherited/TestBufferRemoval3_feedthrough.v",
            "inherited/TestInsertBuffer_BeforeLoads_Case34_pre.v",
            "inherited/TestInsertBuffer_BusBitModNetName_pre.v",
            "inherited/TestReadVerilog_BusBitAndEscapedScalarAreDistinct.v",
            "inherited/get_ports1.v",
            "inherited/hier3.v",
            "lhs_partsel_sub.v",
            "min_ft_one_read_only.v",
            "min_ft_renamed.v",
            "min_ft_shared_inbus.v",
            "min_ft_two_reads_bitwise.v",
            "min_ft_two_reads_concat.v",
            "mined_3ddad16377_feedthrough_port_modnet.v",
            "mixed_paths_sub_and_top.v",
            "nameorder_gate_driven_control.v",
            "nameorder_input_first.v",
            "nameorder_out_before_in.v",
            "nameorder_wire_after.v",
            "out_from_out_sub_to_top.v",
            "out_readback.v",
            "overlap_rhs_sub.v",
            "slicedin_bus_topwire.v",
            "slicedin_scalar_topwire.v",
            "slicedin_split_read.v",
            "structural/bx_bus_geometry_concat_nested_in.v",
            "structural/bx_bus_geometry_inport_scalar_vec.v",
            "structural/bx_dangling_dff_ck_only.v",
            "structural/bx_port_rewiring_header_slice_port.v",
            "structural/bx_port_rewiring_width_leaf_pin_wide_net.v",
            "structural/bx_sequential_probe_sdff.v",
            "structural/bx_sequential_shift8_4mods.v",
            "structural/bx_sequential_shift8_flat.v",
            "structural/bx_sequential_shift8_nest4.v",
            "structural/bx_sequential_two_clk_two_subs.v",
            "structural/s2_portwiring_width_pad_positional_d1.v",
            "structural/wb_sta_reader_header_partselect_port.v",
            "sub_ft_out_loaded_inside.v",
            "sub_in_to_two_outs.v",
            "sub_out_alias_and_gate.v",
            "swap_slices_sub.v",
            "two_subs_chained.v",
            "twoout_ft_topwires.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "top_ports",
        issue = "TBD",
        symptom = "port list reordered and a direction or bus range changed",
        netlists = [
            "structural/wb_sta_reader_header_bitselect_port.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "top_ports",
        issue = "TBD",
        symptom = "port list reordered and a port lost",
        netlists = [
            "bx_naming_escaped_port_kw_input.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # submodule_ports
    #
    # A submodule's port list changed. These are port LOSSES, not reorderings.
    structural_xfail(
        path = "hier",
        check = "submodule_ports",
        issue = "TBD",
        symptom = "port list reordered and a direction or bus range changed",
        netlists = [
            "structural/bx_port_rewiring_explicit_port_concat_scalars.v",
            "structural/bx_port_rewiring_explicit_port_rename_scalar.v",
            "structural/bx_port_rewiring_explicit_port_swap_names.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "submodule_ports",
        issue = "TBD",
        symptom = "port list reordered and a port lost",
        netlists = [
            "bx_naming_escaped_d2_port_kw.v",
            "structural/bx_port_rewiring_explicit_port_bus_halves_crossed.v",
            "structural/bx_port_rewiring_explicit_port_bus_whole.v",
            "structural/bx_port_rewiring_explicit_port_concat_perm.v",
            "structural/bx_port_rewiring_explicit_port_positional_bus.v",
            "structural/bx_port_rewiring_header_concat_port_outputs.v",
            "structural/bx_port_rewiring_header_concat_port_positional.v",
            "structural/bx_port_rewiring_header_concat_port_second.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # declared_nets
    #
    # The set of declared nets and ports in a module changed: dangling nets and
    # dead assign targets are erased, `_NC<n>` filler wires are invented, an
    # undeclared constant net is referenced, or a module-local net comes back
    # named after an instance path.
    structural_xfail(
        path = "flat",
        check = "declared_nets",
        issue = "TBD",
        symptom = "a declared net or port is erased",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "chain2_top.v",
            "chain4_top.v",
            "chain_across_boundary.v",
            "concat_read_ft_wire.v",
            "concat_read_ft_wire_mixed.v",
            "concat_read_scalar_ft.v",
            "concat_read_top_ft.v",
            "concatin_bus_topwire.v",
            "depth2_rename.v",
            "depth4_rename.v",
            "depth4_samename.v",
            "diamond_alias.v",
            "escaped_net_top.v",
            "fanout_two_paths.v",
            "four_subs_chained.v",
            "ft_bitsel_rhs_scalar_out.v",
            "ft_busslice_to_topwire.v",
            "ft_busslice_to_topwire_bitsel.v",
            "ft_busslice_to_topwire_gate.v",
            "ft_busslice_to_wider_slice.v",
            "ft_fullbus_to_topwire.v",
            "ft_scalar_to_topwire.v",
            "gp_bus_wire_two_reads.v",
            "gp_one_out_split_dest.v",
            "gp_scalar_wire_two_reads.v",
            "inherited/TestResizer_SwapPinsFeedthroughModNet_post.v",
            "inherited/TestResizer_SwapPinsFeedthroughModNet_pre.v",
            "min_ft_renamed.v",
            "min_ft_shared_inbus.v",
            "min_ft_two_reads_bitwise.v",
            "min_ft_two_reads_concat.v",
            "nameorder_input_first.v",
            "nameorder_j_after_i.v",
            "nameorder_top_assign_only.v",
            "nameorder_wire_after.v",
            "slicedin_bus_topwire.v",
            "slicedin_concat_read.v",
            "slicedin_scalar_topwire.v",
            "slicedin_split_read.v",
            "structural/bx_constants_assign_z_wire.v",
            "structural/wb_sta_reader_bitselect_out_of_range.v",
            "structural/wb_sta_reader_bus_dcl_initializer.v",
            "structural/wb_sta_reader_scalar_dcl_initializer.v",
            "structural/wb_sta_reader_wor_resolution.v",
            "structural/wb_writer_bitsel_index_stoi_range.v",
            "structural/wb_writer_wire_index_overflow.v",
            "sub_out_alias_and_gate.v",
            "sub_two_outs_to_topwires.v",
            "two_subs_chained.v",
            "twoout_ft_topwires.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "declared_nets",
        issue = "TBD",
        symptom = "a name absent from the input is invented",
        netlists = [
            "structural/wb_sta_reader_hier_ref_dotted_id.v",
        ],
    ),
    # Moved here from the LEC corpus 2026-08-12: 1'bx has no binary value, so a
    # LEC cannot adjudicate it by construction (the oracle refuses with "no
    # aligned observed outputs"). The structural checker sees what write_verilog
    # actually emitted for the x connection, which is what the case was written
    # to record.
    structural_xfail(
        path = "flat",
        check = "declared_nets",
        issue = "TBD",
        symptom = "wire w carrying the x constant is erased from the output",
        netlists = [
            "structural/bx_constants_assign_x_wire.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        issue = "TBD",
        symptom = "_NC filler wires are invented",
        netlists = [
            "structural/bx_bus_geometry_inport_narrower.v",
            "structural/bx_bus_geometry_inport_wider.v",
            "structural/bx_bus_geometry_outport_to_narrow.v",
            "structural/bx_bus_geometry_outport_to_wide.v",
            "structural/bx_constants_width_mismatch.v",
            "structural/bx_port_rewiring_header_slice_port.v",
            "structural/bx_port_rewiring_width_concat_short_of_port.v",
            "structural/bx_port_rewiring_width_narrow_net_to_wide_port.v",
            "structural/bx_port_rewiring_width_off_by_one_port.v",
            "structural/bx_port_rewiring_width_wide_net_to_narrow_port.v",
            "structural/bx_port_rewiring_width_wide_outport_to_narrow_net.v",
            "structural/s2_portwiring_width_pad_positional_d1.v",
            "structural/s2_portwiring_width_trunc_named_d1.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        issue = "TBD",
        symptom = "a declared net or port is erased",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "busslice_mid.v",
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "chain2_sub.v",
            "chain2_top.v",
            "chain4_sub.v",
            "chain4_top.v",
            "chain_across_boundary.v",
            "concat_read_top_ft.v",
            "diamond_alias.v",
            "escaped_net_top.v",
            "escaped_sub_net.v",
            "fanout_two_paths.v",
            "nameorder_top_assign_only.v",
            "structural/bx_constants_assign_z_wire.v",
            "structural/wb_dbnetwork_overlay_stamped_tail_eq_port.v",
            "structural/wb_sta_reader_bitselect_out_of_range.v",
            "structural/wb_sta_reader_scalar_dcl_initializer.v",
            "structural/wb_sta_reader_wor_resolution.v",
            "structural/wb_writer_bitsel_index_stoi_range.v",
            "structural/wb_writer_wire_index_overflow.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        issue = "TBD",
        symptom = "a declared net or port is erased; _NC filler wires are invented",
        netlists = [
            "structural/wb_sta_reader_bus_dcl_initializer.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        issue = "TBD",
        symptom = "a declared net or port is erased; a module-local net is renamed to an instance path",
        netlists = [
            "structural/TestInsertBuffer_BusBitTracePortName_post.v",
            "structural/bx_dangling_undriven_live_sub.v",
            "structural/bx_port_rewiring_header_concat_port_positional.v",
            "structural/bx_port_rewiring_header_concat_port_second.v",
            "structural/wb_dbnetwork_overlay_netname_drvr_escslash.v",
            "structural/wb_dbnetwork_overlay_strip_parent_trailing_bslash.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        issue = "TBD",
        symptom = "a declared net or port is erased; a name absent from the input is invented",
        netlists = [
            "structural/bx_port_rewiring_explicit_port_bus_halves_crossed.v",
            "structural/bx_port_rewiring_explicit_port_bus_whole.v",
            "structural/bx_port_rewiring_explicit_port_concat_perm.v",
            "structural/bx_port_rewiring_explicit_port_positional_bus.v",
            "structural/wb_dbnetwork_overlay_depth0_escslash_rename.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        issue = "TBD",
        symptom = "a name absent from the input is invented",
        netlists = [
            "structural/wb_sta_reader_hier_ref_dotted_id.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        issue = "TBD",
        symptom = "wire w carrying the x constant is erased from the output",
        netlists = [
            "structural/bx_constants_assign_x_wire.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # instances
    #
    # A module's (instance name, master) bindings changed. Mostly instances
    # rebound to uniquification clones; where the master changes to an unrelated
    # cell, the netlist's module identity has been stolen.
    structural_xfail(
        path = "hier",
        check = "instances",
        issue = "TBD",
        symptom = "instances rebound to a uniquified clone or to another master",
        netlists = [
            "busslice_two_insts.v",
            "bx_collisions_base_uniq2.v",
            "bx_collisions_escmod_uniq.v",
            "bx_collisions_inst_named_module_x2.v",
            "bx_collisions_module_prefix_multi.v",
            "bx_collisions_uniq_chain.v",
            "bx_collisions_uniq_clone_eq_libcell.v",
            "bx_collisions_uniq_clone_steals_top.v",
            "bx_collisions_uniq_cross_prefix.v",
            "bx_collisions_uniq_inst_named_sub1.v",
            "bx_collisions_uniq_ports_tied_diff.v",
            "bx_collisions_uniq_resolution_suffix.v",
            "bx_collisions_uniq_same_instname2.v",
            "bx_collisions_uniq_same_instname3.v",
            "bx_collisions_uniq_synth_eq_existing.v",
            "bx_collisions_uniq_theft_order_a.v",
            "bx_collisions_uniq_theft_order_z.v",
            "bx_collisions_uniq_theft_victim_below.v",
            "bx_collisions_uniq_user_mod_below_top.v",
            "bx_collisions_uniq_vs_module_collide.v",
            "bx_collisions_uniq_vs_module_collide_rev.v",
            "bx_collisions_uniq_vs_module_same.v",
            "bx_collisions_uniq_wire_named_clone.v",
            "bx_naming_plain_case_insts_d3.v",
            "escaped_hier_names.v",
            "fanout_two_subs.v",
            "four_subs_chained.v",
            "inherited/TestReadVerilog_DeepDescendantModBTermCollision.v",
            "inherited/modnet_port_alias.v",
            "structural/bx_collisions_escaped_equals_plain_inst.v",
            "structural/bx_collisions_uniq_vs_module_uninst.v",
            "structural/bx_sequential_shift8_4mods.v",
            "structural/wb_dbnetwork_overlay_strip_bslash_inst_dup.v",
            "structural/wb_dbnetwork_overlay_strip_parent_trailing_bslash.v",
            "structural/wb_sta_reader_ifdef_body_compiled.v",
            "two_subs_chained.v",
            "uniquified_module_collision.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # name_identity
    #
    # An emitted module name no longer maps back to exactly one input module.
    # This is the assertion module_set and instances cannot make: those two fire
    # on every uniquification, benign or not, so a row keyed on them proves
    # nothing about the cases below, where the name-to-module mapping is
    # actually destroyed -- a clone name that is the uniquification name of two
    # different modules at once, or a clone that takes a real module's name and
    # leaves the name pointing at the wrong module.
    structural_xfail(
        path = "hier",
        check = "name_identity",
        issue = "TBD",
        symptom = "a clone takes an existing module name, so the name denotes the wrong module",
        netlists = [
            "bx_collisions_uniq_chain.v",
            "bx_collisions_uniq_theft_order_z.v",
            "bx_collisions_uniq_theft_victim_below.v",
            "bx_collisions_uniq_vs_module_collide.v",
            "bx_collisions_uniq_vs_module_collide_rev.v",
            "bx_collisions_uniq_vs_module_same.v",
            "structural/bx_collisions_uniq_vs_module_uninst.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "name_identity",
        issue = "TBD",
        symptom = "one clone name is the uniquification name of two different modules",
        netlists = [
            "bx_collisions_uniq_cross_prefix.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # cell_census
    #
    structural_xfail(
        path = "hier",
        check = "cell_census",
        issue = "TBD",
        symptom = "the elaborated leaf instance count changed",
        netlists = [
            "bx_collisions_uniq_clone_eq_libcell.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # assigns
    #
    # Continuous assigns are dropped (a dead assign, or an alias chain collapsed
    # by flattening), or one is added next to a connection that already drives
    # the same net.
    structural_xfail(
        path = "flat",
        check = "assigns",
        issue = "TBD",
        symptom = "a continuous assign is dropped",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "chain2_top.v",
            "chain4_top.v",
            "chain_across_boundary.v",
            "concat_read_top_ft.v",
            "diamond_alias.v",
            "escaped_net_top.v",
            "fanout_two_paths.v",
            "nameorder_top_assign_only.v",
            "structural/bx_constants_assign_z_wire.v",
            "structural/wb_sta_reader_assign_width_zero_extend.v",
            "structural/wb_sta_reader_bus_dcl_initializer.v",
            "structural/wb_sta_reader_scalar_dcl_initializer.v",
            "structural/wb_sta_reader_wor_resolution.v",
        ],
    ),
    # Moved here from the LEC corpus 2026-08-12: 1'bx has no binary value, so a
    # LEC cannot adjudicate it by construction (the oracle refuses with "no
    # aligned observed outputs"). The structural checker sees what write_verilog
    # actually emitted for the x connection, which is what the case was written
    # to record.
    structural_xfail(
        path = "flat",
        check = "assigns",
        issue = "TBD",
        symptom = "the assign driving the x constant is dropped with its wire",
        netlists = [
            "structural/bx_constants_assign_x_wire.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "assigns",
        issue = "TBD",
        symptom = "a continuous assign is dropped",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "busslice_mid.v",
            "chain2_sub.v",
            "chain2_top.v",
            "chain4_sub.v",
            "chain4_top.v",
            "chain_across_boundary.v",
            "concat_read_top_ft.v",
            "diamond_alias.v",
            "escaped_net_top.v",
            "escaped_sub_net.v",
            "fanout_two_paths.v",
            "nameorder_top_assign_only.v",
            "structural/bx_constants_assign_z_wire.v",
            "structural/wb_sta_reader_assign_width_zero_extend.v",
            "structural/wb_sta_reader_bus_dcl_initializer.v",
            "structural/wb_sta_reader_scalar_dcl_initializer.v",
            "structural/wb_sta_reader_wor_resolution.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "assigns",
        issue = "TBD",
        symptom = "an extra continuous assign is added",
        netlists = [
            "busslice_same_in_two_outs.v",
            "fanout_two_subs.v",
            "inherited/TestInsertBuffer_BeforeLoads_Case33_post.v",
            "overlap_rhs_sub.v",
            "sub_in_to_two_outs.v",
            "sub_out_from_out.v",
            "sub_out_from_out_bus.v",
            "sub_out_from_out_deep3.v",
            "sub_three_outs_one_driver.v",
            "sub_two_outs_one_driver.v",
            "sub_two_outs_one_to_gate.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "assigns",
        issue = "TBD",
        symptom = "the assign driving the x constant is dropped with its wire",
        netlists = [
            "structural/bx_constants_assign_x_wire.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # namespace
    #
    # The emitted netlist breaks Verilog's one-namespace-per-module rule, or
    # emits an identifier that needed an escape without it. These netlists are
    # not readable by a conforming tool, and OpenROAD itself silently merges the
    # duplicate on re-read.
    structural_xfail(
        path = "flat",
        check = "namespace",
        issue = "TBD",
        symptom = "a name is declared twice in one module",
        netlists = [
            "bx_collisions_dff_net_vs_esc.v",
            "bx_collisions_net_deep3.v",
            "bx_collisions_net_vs_flatnet.v",
            "bx_collisions_subnet_esc_vs_path.v",
            "bx_collisions_synth_net.v",
            "bx_collisions_unconn_port_vs_esc.v",
            "bx_naming_escaped_slashcol_escnet.v",
            "bx_naming_escaped_slashcol_net.v",
            "bx_naming_escaped_slashcol_port.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "namespace",
        issue = "TBD",
        symptom = "an identifier is emitted without the escape it needs",
        netlists = [
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "structural/wb_writer_wire_index_overflow.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "namespace",
        issue = "TBD",
        symptom = "one name is used for a net and an instance",
        netlists = [
            "bx_collisions_inst_vs_net_cross.v",
            "bx_collisions_net_and_inst_same_name.v",
            "bx_collisions_net_vs_inst_cross.v",
            "bx_collisions_outport_vs_instpath.v",
            "bx_collisions_topport_escaped.v",
            "bx_naming_escaped_slashcol_instnet.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "namespace",
        issue = "TBD",
        symptom = "an identifier is emitted without the escape it needs",
        netlists = [
            "bx_naming_escaped_d2_port_kw.v",
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "structural/wb_writer_wire_index_overflow.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "namespace",
        issue = "TBD",
        symptom = "one name is used for a net and an instance",
        netlists = [
            "bx_collisions_net_and_inst_same_name.v",
        ],
    ),
]
