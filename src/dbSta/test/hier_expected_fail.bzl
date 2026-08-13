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
            "inherited/TestReadVerilog_DeepDescendantModBTermCollision.v",
            "inherited/modnet_port_alias.v",
            "structural/bx_sequential_shift8_4mods.v",
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
            "inherited/TestBufferRemoval3_feedthrough.v",
            "inherited/TestInsertBuffer_BeforeLoads_Case34_pre.v",
            "inherited/TestInsertBuffer_BusBitModNetName_pre.v",
            "inherited/TestReadVerilog_BusBitAndEscapedScalarAreDistinct.v",
            "inherited/get_ports1.v",
            "inherited/hier3.v",
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
        path = "hier",
        check = "top_ports",
        issue = "TBD",
        symptom = "port list reordered",
        netlists = [
            "inherited/TestBufferRemoval3_feedthrough.v",
            "inherited/TestInsertBuffer_BeforeLoads_Case34_pre.v",
            "inherited/TestInsertBuffer_BusBitModNetName_pre.v",
            "inherited/TestReadVerilog_BusBitAndEscapedScalarAreDistinct.v",
            "inherited/get_ports1.v",
            "inherited/hier3.v",
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
            "inherited/TestResizer_SwapPinsFeedthroughModNet_post.v",
            "inherited/TestResizer_SwapPinsFeedthroughModNet_pre.v",
            "structural/bx_constants_assign_z_wire.v",
            "structural/wb_sta_reader_bitselect_out_of_range.v",
            "structural/wb_sta_reader_bus_dcl_initializer.v",
            "structural/wb_sta_reader_scalar_dcl_initializer.v",
            "structural/wb_sta_reader_wor_resolution.v",
            "structural/wb_writer_bitsel_index_stoi_range.v",
            "structural/wb_writer_wire_index_overflow.v",
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
            "inherited/TestReadVerilog_DeepDescendantModBTermCollision.v",
            "inherited/modnet_port_alias.v",
            "structural/bx_collisions_escaped_equals_plain_inst.v",
            "structural/bx_collisions_uniq_vs_module_uninst.v",
            "structural/bx_sequential_shift8_4mods.v",
            "structural/wb_dbnetwork_overlay_strip_bslash_inst_dup.v",
            "structural/wb_dbnetwork_overlay_strip_parent_trailing_bslash.v",
            "structural/wb_sta_reader_ifdef_body_compiled.v",
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
            "structural/bx_collisions_uniq_vs_module_uninst.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # cell_census
    #

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
            "inherited/TestInsertBuffer_BeforeLoads_Case33_post.v",
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
    structural_xfail(
        path = "flat",
        check = "namespace",
        issue = "TBD",
        symptom = "an identifier is emitted without the escape it needs",
        netlists = [
            "structural/wb_writer_wire_index_overflow.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "namespace",
        issue = "TBD",
        symptom = "an identifier is emitted without the escape it needs",
        netlists = [
            "structural/wb_writer_wire_index_overflow.v",
        ],
    ),
]
