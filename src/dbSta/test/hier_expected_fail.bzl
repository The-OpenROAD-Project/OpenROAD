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

A conformance entry also records *how* the case fails, and the suite holds it to
that: an XFAIL that accepts any failure would keep passing after a
counterexample turned into a crash. See _MODES.

An entry may cite an OpenROAD issue, and should once one exists, so that
"expected to fail" does not become a shrug. It is optional because the alternative
is a file full of "TBD" -- a required field with nothing to put in it teaches
everyone to write a placeholder, which is worse than an empty one: the reader
cannot tell a defect nobody has filed from one whose number was never
backfilled.

src/dbSta/test/BUILD renders these into the manifests the two suites read.
"""

_PATHS = ["hier", "flat"]

# How a conformance case fails. TestHierConformance.cpp derives the same tokens
# from what it observes, and compares. Recording the mode is what keeps an XFAIL
# from degrading into "this case is allowed to be broken in any way at all":
# with it, a case that starts failing differently is a finding.
_MODES = [
    # read_verilog or link_design rejected the netlist (an OpenROAD error).
    "or-error",
    # write_verilog threw.
    "write-error",
    # The emitted netlist is not equivalent to the input.
    "counterexample",
    # The two designs' boundary sets differ, so the check never ran -- usually a
    # dropped or renamed port.
    "boundary-mismatch",
    # Proved, but not over every output: a dropped connection leaves outputs in
    # a cone with no driver, and those are skipped rather than compared.
    "partial",
    # The checker produced no verdict we can read.
    "inconclusive",
    # The checker could not run on the pair at all, most often because the
    # emitted netlist does not parse.
    "tool-error",
]

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

# Values that mean "no issue" while looking like one. Rejected so that the field
# is either a real reference or visibly absent, never a placeholder that outlives
# whoever wrote it.
_NON_ISSUES = ["tbd", "todo", "n/a", "na", "none", "?", "-", "xxx", "fixme"]

def _validate(path, issue, symptom, netlists, check = None, mode = None):
    if path not in _PATHS:
        fail("unknown path '%s'; expected one of %s" % (path, _PATHS))
    if check != None and check not in _CHECKS:
        fail("unknown check '%s'; expected one of %s" % (check, _CHECKS))
    if mode != None and mode not in _MODES:
        fail("unknown mode '%s'; expected one of %s" % (mode, _MODES))
    if not netlists:
        fail("xfail for '%s' lists no netlists" % symptom)
    if issue != None and issue.lower() in _NON_ISSUES:
        fail("'%s' is a placeholder, not an issue reference; omit `issue` " %
             issue + "until one is filed for '%s'" % symptom)

    # The rendered manifest is colon-separated, so a colon anywhere in a field
    # would silently truncate the entry when the test parses it back.
    for field in [path, symptom, issue or "", mode or ""] + netlists:
        if ":" in field:
            fail("':' is the manifest field separator, so it cannot appear " +
                 "in '%s'" % field)

# '*' matches any run of characters; nothing else is special. Matching lives here
# rather than in the suite because the rows a case needs are selected when the
# package loads: a per-case test target carries its own rows and never reads the
# whole manifest, so nothing downstream has a pattern left to match.
#
# Split on '*' rather than walking the string: Starlark has no while loop, and
# "a*b*c" matches exactly when the text starts with "a", ends with "c", and the
# middle pieces occur in order.
def _glob_match(pattern, text):
    pieces = pattern.split("*")
    if len(pieces) == 1:
        return pattern == text
    if not text.startswith(pieces[0]):
        return False
    pos = len(pieces[0])
    for piece in pieces[1:-1]:
        if not piece:
            continue
        found = text.find(piece, pos)
        if found == -1:
            return False
        pos = found + len(piece)
    last = pieces[-1]
    if not last:
        return True
    return text.endswith(last) and len(text) - len(last) >= pos

# The corpus netlists a pattern names. A pattern that names none is an error: it
# is dead text that still reads as coverage of a known defect, which is how an
# entry outlives the case it was written for. The conformance suite reports the
# same thing at run time (ManifestIsWellFormed); the structural suite cannot,
# because a pattern makes "is this row used?" unanswerable there.
def _expand(pattern, corpus, symptom):
    if "*" not in pattern:
        if pattern not in corpus:
            fail(("'%s' is listed for '%s', but no such netlist is in the " +
                  "corpus. Remove the entry, or restore the case it names.") %
                 (pattern, symptom))
        return [pattern]
    matched = [netlist for netlist in corpus if _glob_match(pattern, netlist)]
    if not matched:
        fail(("'%s' is listed for '%s', but it matches no netlist in the " +
              "corpus. Remove the entry, or widen it.") % (pattern, symptom))
    return matched

# A netlist can only have one recorded symptom per key: the test reads the first
# matching row, so a second one would be dead text that still reads as coverage.
def _reject_duplicate(seen, key, symptom):
    if key in seen:
        fail("'%s' is listed twice, as '%s' and as '%s'; one entry wins and " %
             (key, seen[key], symptom) + "the other is never reported")
    seen[key] = symptom

def xfail(path, mode, symptom, netlists, issue = None):
    """One conformance failure mode.

    Args:
      path: "hier" or "flat" -- which link mode fails. Keyed on (netlist, path)
        because the hier path can be broken while the flat path is clean, which
        is the expected shape of a finding here.
      mode: how the case fails; one of _MODES. The suite fails the case if it
        stops failing this way, so every netlist listed must share it -- split
        the entry rather than widening it.
      symptom: what the LEC reported, short enough to read in a failure message.
      netlists: the netlists that fail this way, by file name.
      issue: the OpenROAD issue number, if one has been filed.

    Returns:
      A struct the BUILD file renders into a manifest line per netlist.
    """
    _validate(path, issue, symptom, netlists, mode = mode)
    return struct(
        path = path,
        mode = mode,
        issue = issue,
        symptom = symptom,
        netlists = netlists,
    )

def structural_xfail(path, check, symptom, netlists, issue = None):
    """One structural failure mode.

    Args:
      path: "hier" or "flat" -- which link mode fails.
      check: which comparison fails; one of _CHECKS.
      symptom: what the structural diff reported.
      netlists: the netlists that fail this way. A netlist from the
        hier_cases/structural/ subdirectory keeps that prefix in its key
        ("structural/case.v"). A name may be a glob ('*' matches any run of
        characters), so a systemic defect that hits the whole corpus is one
        entry rather than 378 -- XPASS semantics are unaffected, since each case
        tests its own key.
      issue: the OpenROAD issue number, if one has been filed.

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

def conformance_manifest_by_netlist(entries):
    """Groups entries as netlist -> `netlist : path : mode : issue : symptom`.

    Args:
      entries: xfail() structs.

    Returns:
      A dict of netlist name to its rows, so a per-case target can carry the
      rows that name it and nothing else.
    """
    rows = {}
    seen = {}
    for e in entries:
        for netlist in e.netlists:
            key = netlist + " : " + e.path
            _reject_duplicate(seen, key, e.symptom)

            # Every field is emitted even when empty, so the row keeps a fixed
            # arity and the reader never has to guess which field is missing.
            rows.setdefault(netlist, []).append(
                " : ".join([netlist, e.path, e.mode, e.issue or "", e.symptom]),
            )
    return rows

def structural_manifest_by_netlist(entries, corpus):
    """Groups entries as netlist -> `... : issue : symptom : as_authored`.

    Patterns are expanded against `corpus` here, so every row names one netlist
    exactly and the suite compares names rather than matching. The pattern as
    authored is kept as the last field: it is what a failure message must tell
    the reader to go and delete.

    Args:
      entries: structural_xfail() structs.
      corpus: every corpus-relative netlist name, e.g. "structural/case.v".

    Returns:
      A dict of netlist name to its rows.
    """
    rows = {}
    seen = {}
    for e in entries:
        for pattern in e.netlists:
            for netlist in _expand(pattern, corpus, e.symptom):
                key = " : ".join([netlist, e.path, e.check])
                _reject_duplicate(seen, key, e.symptom)
                rows.setdefault(netlist, []).append(" : ".join([
                    netlist,
                    e.path,
                    e.check,
                    e.issue or "",
                    e.symptom,
                    pattern,
                ]))
    return rows

def manifest_lines(rows_by_netlist, corpus):
    """Flattens grouped rows into a manifest, in corpus order.

    Args:
      rows_by_netlist: the dict from one of the by_netlist functions above.
      corpus: every corpus-relative netlist name, for a stable row order.

    Returns:
      One line per row, for write_file.
    """
    lines = []
    for netlist in corpus:
        lines.extend(rows_by_netlist.get(netlist, []))

    # A row naming no corpus netlist is emitted too, at the end: dropping it
    # here would hide it from the corpus-wide check that exists to report it
    # (ManifestIsWellFormed). The structural manifest cannot contain one, since
    # _expand rejects it when the package loads.
    for netlist, rows in rows_by_netlist.items():
        if netlist not in corpus:
            lines.extend(rows)
    return lines

# Failures of the LEC round trip: read_verilog -> link_design [-hier] ->
# write_verilog, proved against the input netlist. Grouped by symptom, and
# within a group sorted by name.
CONFORMANCE_EXPECTED_FAIL = [
    xfail(
        path = "flat",
        mode = "counterexample",
        symptom = "counterexample",
        netlists = [
            "bx_constants_esc_subzero_net_buf.v",
            "bx_constants_port_named_zero.v",
            "bx_constants_sub_zero_capture.v",
            "bx_constants_user_one_wire.v",
            "bx_constants_user_zero_wire.v",
            "wb_sta_reader_module_shadows_cell_after.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "counterexample",
        symptom = "counterexample",
        netlists = [
            "bx_constants_port_named_zero.v",
            "bx_constants_sub_zero_capture.v",
            "bx_constants_user_one_wire.v",
            "bx_constants_user_zero_wire.v",
            "wb_dbsta_link_escslash_port_tail_after.v",
            "wb_dbsta_link_escslash_port_tail_depth2.v",
            "wb_sta_reader_module_shadows_cell_after.v",
        ],
    ),
    # The flat writer drops sub_module's `assign out_bus = in_bus[3:2];`
    # entirely, leaving sub_out_bus undriven and taking top_out_bus[1] and
    # top_out_single with it (SEC coverage 4/6). The hier path emits the assigns
    # correctly.
    xfail(
        path = "flat",
        mode = "partial",
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
        mode = "tool-error",
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
        mode = "tool-error",
        symptom = "hier write adds a duplicate driver on an already-driven output port",
        netlists = [
            "TestInsertBuffer_BeforeLoads_Case33_post.v",
        ],
    ),
    # Not an OpenROAD defect. Under dual_rail_steady the oracle refuses any pair
    # containing an integrated clock gate: "SNLLogicCloud arity mismatch for
    # model CLKGATE_X1 -- TT arity=0, model non-output term count=2". The
    # netlists round-trip, and the same files were previously "proved" only
    # because the older binary abstracted the CLKGATE latch away as an
    # uncomputable sequential, so clock-gate semantics were never actually
    # checked. Delete this entry once kepler models CLKGATE_X1.
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "kepler cannot model CLKGATE_X1 (arity mismatch)",
        netlists = [
            "bx_sequential_probe_clkgate.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "kepler cannot model CLKGATE_X1 (arity mismatch)",
        netlists = [
            "bx_sequential_probe_clkgate.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "Error - out_flat.tcl, 3 stol - no conversion",
        netlists = [
            "bx_constants_unsized_b0.v",
            "bx_constants_unsized_d1.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "Error - out_flat.tcl, 4 stoi - no conversion",
        netlists = [
            "wb_dbsta_link_attr_dont_touch_string.v",
            "wb_sta_reader_attr_dont_touch_string.v",
            "wb_sta_reader_attr_survival.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "Error - out_flat.tcl, 4 stoi - out of range",
        netlists = [
            "wb_sta_reader_attr_src_line_overflow.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "Error - out_hier.tcl, 3 stol - no conversion",
        netlists = [
            "bx_constants_unsized_b0.v",
            "bx_constants_unsized_d1.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "Error - out_hier.tcl, 4 stoi - no conversion",
        netlists = [
            "wb_dbsta_link_attr_dont_touch_string.v",
            "wb_sta_reader_attr_dont_touch_string.v",
            "wb_sta_reader_attr_survival.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "Error - out_hier.tcl, 4 stoi - out of range",
        netlists = [
            "wb_sta_reader_attr_src_line_overflow.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "STA-0171-syntax-error",
        netlists = [
            "bx_bus_geometry_concat_replicate.v",
            "bx_bus_geometry_replicate_assign.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "STA-0171-syntax-error",
        netlists = [
            "bx_bus_geometry_concat_replicate.v",
            "bx_bus_geometry_replicate_assign.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR ORD-2013] instance u LEF master missing_mod not found.",
        netlists = [
            "wb_dbsta_link_unresolved_module.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR ORD-2013] instance u LEF master missing_mod not found.",
        netlists = [
            "wb_dbsta_link_unresolved_module.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...bx_dangling_positional_all_empty.v line 14, syntax error",
        netlists = [
            "bx_dangling_positional_all_empty.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...bx_dangling_positional_hole_leaf.v line 9, syntax error",
        netlists = [
            "bx_dangling_positional_hole_leaf.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...bx_dangling_subin_positional_hole.v line 16, syntax error",
        netlists = [
            "bx_dangling_subin_positional_hole.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...bx_dangling_subin_positional_trailing.v line 16, syntax error",
        netlists = [
            "bx_dangling_subin_positional_trailing.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...bx_dangling_subout_positional_hole.v line 17, syntax error",
        netlists = [
            "bx_dangling_subout_positional_hole.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...line 14, syntax error",
        netlists = [
            "bx_dangling_positional_all_empty.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...line 16, syntax error",
        netlists = [
            "bx_dangling_subin_positional_hole.v",
            "bx_dangling_subin_positional_trailing.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...line 17, syntax error",
        netlists = [
            "bx_dangling_subout_positional_hole.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] ...line 9, syntax error",
        netlists = [
            "bx_dangling_positional_hole_leaf.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] /home/pgadfort/hier-lec-artifacts/recovered/survey/sta_reader/wb_sta_reader_attr_bef",
        netlists = [
            "wb_sta_reader_attr_before_assign.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] /home/pgadfort/hier-lec-artifacts/recovered/survey/sta_reader/wb_sta_reader_attr_bef",
        netlists = [
            "wb_sta_reader_attr_before_assign.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] line 10, syntax error",
        netlists = [
            "bx_port_rewiring_positional_gap_input.v",
            "bx_port_rewiring_positional_gap_last.v",
            "bx_port_rewiring_positional_gap_output.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] line 10, syntax error",
        netlists = [
            "bx_port_rewiring_positional_gap_input.v",
            "bx_port_rewiring_positional_gap_last.v",
            "bx_port_rewiring_positional_gap_output.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] line 15 syntax error",
        netlists = [
            "bx_constants_repl_concat.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] line 15 syntax error",
        netlists = [
            "bx_constants_repl_concat.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] line 16 syntax error",
        netlists = [
            "bx_constants_repl_signal.v",
            "bx_constants_repl_simple.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] line 16 syntax error",
        netlists = [
            "bx_constants_repl_signal.v",
            "bx_constants_repl_simple.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "or-error",
        symptom = "[ERROR STA-0171] syntax error",
        netlists = [
            "bx_port_rewiring_concat_replication_port_conn.v",
            "bx_port_rewiring_instance_array_bus_split.v",
            "bx_port_rewiring_positional_gap_first.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "or-error",
        symptom = "[ERROR STA-0171] syntax error",
        netlists = [
            "bx_port_rewiring_concat_replication_port_conn.v",
            "bx_port_rewiring_instance_array_bus_split.v",
            "bx_port_rewiring_positional_gap_first.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "partial",
        symptom = "SEC coverage 20.00%",
        netlists = [
            "bx_bus_geometry_concat_all_const.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "partial",
        symptom = "SEC coverage 20.00%",
        netlists = [
            "bx_bus_geometry_concat_all_const.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "partial",
        symptom = "SEC coverage 25.00%",
        netlists = [
            "getports_wholein.v",
            "gp_bitassign_top.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "partial",
        symptom = "SEC coverage 33.33%",
        netlists = [
            "gp_no_bus_ft.v",
            "gp_no_scalar_ft.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "partial",
        symptom = "SEC coverage 33.33%",
        netlists = [
            "sub_three_outs_one_driver.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "partial",
        symptom = "SEC coverage 50.00%",
        netlists = [
            "bx_bus_geometry_concat_const_mix.v",
            "bx_bus_geometry_const_gatepin_top.v",
            "bx_bus_geometry_const_scalar_port.v",
            "bx_constants_mixed_cell_literal.v",
            "getports_nocell.v",
            "min_ft_one_read_only.v",
            "nameorder_wire_before.v",
            "wb_dbsta_link_supply_net_hier_boundary.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "partial",
        symptom = "SEC coverage 50.00%",
        netlists = [
            "bx_bus_geometry_concat_const_mix.v",
            "bx_bus_geometry_const_gatepin_top.v",
            "bx_bus_geometry_const_scalar_port.v",
            "bx_bus_geometry_negasc_child.v",
            "bx_bus_geometry_negdesc_child.v",
            "bx_constants_esc_subzero_net.v",
            "bx_constants_esc_subzero_net_buf.v",
            "bx_constants_mixed_cell_literal.v",
            "bx_constants_sub_tiehi_sibling.v",
            "sub_two_outs_one_driver.v",
            "sub_two_outs_one_to_gate.v",
            "wb_dbsta_link_supply_net_hier_boundary.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "partial",
        symptom = "SEC coverage 60.00%",
        netlists = [
            "busslice_same_in_two_outs.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "partial",
        symptom = "SEC coverage 66.67%",
        netlists = [
            "bx_constants_assign_out_bitsel.v",
            "getports_bitassign.v",
            "getports_replica.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "partial",
        symptom = "SEC coverage 66.67%",
        netlists = [
            "bx_constants_assign_out_bitsel.v",
            "fanout_two_subs.v",
            "sub_in_to_two_outs.v",
            "wb_writer_nc_drift_captures_user_net.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "partial",
        symptom = "SEC coverage 80.00%",
        netlists = [
            "overlap_rhs_sub.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "'SNLDesign top contains already a SNLInstance named - x/y' — exact known pattern",
        netlists = [
            "bx_naming_escaped_slashcol_inst.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "'SNLDesign top contains already a SNLInstance named - x/y/z' — depth-3 path variant",
        netlists = [
            "bx_naming_escaped_slashcol_inst3.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "duplicate 'wire \\u1/w+w ;' — escaped sub net \\w+w flattened to u1/w+w collides with top escaped ne",
        netlists = [
            "bx_naming_escaped_slashcol_escnet.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "duplicate 'wire \\x/p ;' — net synthesized for unconnected output port p of instance x collides wit",
        netlists = [
            "bx_naming_escaped_slashcol_port.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "emitted 'INV_X1 assign (...)' unescaped, 'unexpected ASSIGN_KW'",
        netlists = [
            "bx_naming_escaped_inst_kw_assign.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "emitted 'module module (' — top module keyword name unescaped",
        netlists = [
            "bx_naming_escaped_top_kw.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "emitted 'module top (input, z); input input;' unescaped — illegal",
        netlists = [
            "bx_naming_escaped_port_kw_input.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "emitted netlist illegal, 'unexpected MODULE_KW' (writer emitted 'wire module;')",
        netlists = [
            "bx_naming_escaped_net_kw_module.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "emitted netlist illegal, 'unexpected WIRE_KW' (writer emitted 'wire wire;')",
        netlists = [
            "bx_naming_escaped_net_kw_wire.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "hier writer emitted 'INV_X1 assign (...)' inside submodule — illegal; flat proved only because the",
        netlists = [
            "bx_naming_escaped_d2_inst_kw.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "hier writer emitted 'module module (' and 'module u1 (...)' unescaped — illegal; flat proved becau",
        netlists = [
            "bx_naming_escaped_mod_kw.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "hier writer emitted 'output output;' and '.output(z)' unescaped — illegal; flat proved only becaus",
        netlists = [
            "bx_naming_escaped_d2_port_kw.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "out_flat.v declares 'wire \\x/y ;' TWICE with two different drivers (top escaped net vs flattened s",
        netlists = [
            "bx_naming_escaped_slashcol_net.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "same",
        netlists = [
            "bx_naming_escaped_inst_kw_assign.v",
            "bx_naming_escaped_port_kw_input.v",
            "bx_naming_escaped_top_kw.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "same, 'wire module;' unescaped",
        netlists = [
            "bx_naming_escaped_net_kw_module.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "same, 'wire wire;' unescaped",
        netlists = [
            "bx_naming_escaped_net_kw_wire.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "A port cannot be found in INV_X1 model (out_hier.v line 22)",
        netlists = [
            "bx_collisions_uniq_clone_eq_libcell.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "Netlist loading failed - ... out_flat.v at line 11, column 2 - wire collision for net u/c/ckb",
        netlists = [
            "bx_sequential_esc_clknet_collide_d2.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "Netlist loading failed - ... out_flat.v at line 11, column 2 - wire collision for net u/ckb",
        netlists = [
            "bx_sequential_esc_clknet_collide.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "Netlist loading failed - ... out_flat.v at line 11, column 2 - wire collision for net u/qi",
        netlists = [
            "bx_sequential_esc_qnet_collide.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "Netlist loading failed - ... wire collision for net ab",
        netlists = [
            "wb_dbnetwork_overlay_netname_header_single_bslash.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "Netlist loading failed - ... wire collision for net n",
        netlists = [
            "wb_dbnetwork_overlay_dcflat_hier_mixed.v",
            "wb_dbnetwork_overlay_dcflat_pathnames_short.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "Netlist loading failed - ... wire collision for net u1/w",
        netlists = [
            "wb_dbnetwork_overlay_stamped_path_dup_decl.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "Netlist loading failed - ... wire collision for net x",
        netlists = [
            "wb_dbnetwork_overlay_erase_overshoot_net_victim.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "Netlist loading failed - wire collision for net _NC1",
        netlists = [
            "bx_dangling_busport_ncname_collision.v",
            "bx_dangling_nc_collision_bus.v",
            "bx_dangling_nc_collision_mid.v",
            "bx_dangling_nc_collision_scalar_live.v",
            "bx_dangling_nc_filler_all4_taken.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "Netlist loading failed - wire collision for net _NC2",
        netlists = [
            "bx_dangling_busport_nc2_collision.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "Netlist loading failed - wire collision for net u1/und",
        netlists = [
            "bx_dangling_undriven_leak_collide.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SEC cannot run - Missing observed output expression",
        netlists = [
            "bx_port_rewiring_leaf_positional_buf.v",
            "bx_port_rewiring_leaf_positional_inv.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "SEC cannot run - Missing observed output expression",
        netlists = [
            "bx_port_rewiring_leaf_positional_buf.v",
            "bx_port_rewiring_leaf_positional_inv.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SEC cannot run - No aligned observed outputs remain",
        netlists = [
            "bx_port_rewiring_leaf_positional_aoi21.v",
            "bx_port_rewiring_leaf_positional_dff.v",
            "bx_port_rewiring_leaf_positional_fa.v",
            "bx_port_rewiring_leaf_positional_ha.v",
            "bx_port_rewiring_leaf_positional_in_permuting_child.v",
            "bx_port_rewiring_leaf_positional_mux2.v",
            "bx_port_rewiring_leaf_positional_nand2.v",
            "bx_port_rewiring_leaf_positional_xor2_inside_hier.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "SEC cannot run - No aligned observed outputs remain",
        netlists = [
            "bx_port_rewiring_leaf_positional_aoi21.v",
            "bx_port_rewiring_leaf_positional_dff.v",
            "bx_port_rewiring_leaf_positional_fa.v",
            "bx_port_rewiring_leaf_positional_ha.v",
            "bx_port_rewiring_leaf_positional_in_permuting_child.v",
            "bx_port_rewiring_leaf_positional_mux2.v",
            "bx_port_rewiring_leaf_positional_nand2.v",
            "bx_port_rewiring_leaf_positional_xor2_inside_hier.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SEC cannot run - No aligned observed outputs remain after skipping cones with no-driver, multi-drive",
        netlists = [
            "wb_dbsta_link_supply_net_sigtype.v",
        ],
    ),
    # The escaped bus `\x/y [1:0]` yields bit names that tail-match the plain
    # bus `y[1:0]` created before it, and dbBusPort::create re-parents y's
    # aggregate sentinel to the new port -- one bus port swallows the other and
    # the two inputs come back crossed.
    xfail(
        path = "hier",
        mode = "counterexample",
        symptom = "an escaped bus port and a plain bus port are wired to each other's nets",
        netlists = [
            "wb_dbsta_link_escslash_bus_tail.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "SEC cannot run - No aligned observed outputs remain after skipping cones with no-driver, multi-drive",
        netlists = [
            "wb_dbsta_link_escslash_port_tail_iodir.v",
            "wb_dbsta_link_supply_net_sigtype.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SEC cannot run - no aligned observed outputs remain after skipping multi-driver cones",
        netlists = [
            "bx_collisions_outport_vs_flatnet.v",
            "bx_collisions_port_in_vs_flatnet.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SEC cannot run on this design pair - Missing observed output expression for `197.0.`",
        netlists = [
            "nameorder_out_before_in.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SEC cannot run on this design pair - No aligned observed outputs remain after skipping cones with no",
        netlists = [
            "bx_dangling_positional_inv_live.v",
            "bx_dangling_positional_leaf_live.v",
            "gp_full_inbus.v",
            "gp_no_topin_in_concat.v",
            "nameorder_busslice.v",
            "nameorder_deep_chain.v",
            "nameorder_h_before_i.v",
            "nameorder_minimal_repro.v",
            "wb_sta_reader_supply_tie.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "SEC cannot run on this design pair - No aligned observed outputs remain after skipping cones with no",
        netlists = [
            "sub_out_from_out.v",
            "sub_out_from_out_bus.v",
            "sub_out_from_out_deep3.v",
            "wb_dbnetwork_overlay_depth0_escslash_rename_port.v",
            "wb_dbnetwork_overlay_netname_erase_overshoot_port.v",
            "wb_sta_reader_supply_tie.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "SEC cannot run on this design pair - No aligned observed outputs remain...",
        netlists = [
            "bx_dangling_positional_inv_live.v",
            "bx_dangling_positional_leaf_live.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - a/b/c",
        netlists = [
            "bx_collisions_inst_deep3.v",
            "bx_collisions_synth_inst.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - m/c/d",
        netlists = [
            "bx_collisions_submodule_esc_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - x/y",
        netlists = [
            "bx_collisions_known3_esc_first.v",
            "bx_collisions_known3_inst_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - x/y/z",
        netlists = [
            "bx_collisions_inst_mixed_esc.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "SNLVRLConstructor - SNLDesign top contains already a SNLInstance named - x/y[0]",
        netlists = [
            "bx_collisions_escbracket_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "boundary-mismatch",
        symptom = "boundary-input-set-mismatch",
        netlists = [
            "bx_bus_geometry_neg_partsel.v",
            "bx_bus_geometry_neg_top.v",
            "bx_bus_geometry_negonly_top.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "boundary-mismatch",
        symptom = "boundary-input-set-mismatch",
        netlists = [
            "bx_bus_geometry_neg_partsel.v",
            "bx_bus_geometry_neg_top.v",
            "bx_bus_geometry_negonly_top.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "emitted-verilog-syntax-error",
        netlists = [
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "emitted-verilog-syntax-error",
        netlists = [
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "missing observed output expression",
        netlists = [
            "bx_constants_alias_out.v",
            "bx_constants_assign_busout_top.v",
            "bx_constants_assign_out_top.v",
            "bx_constants_concat_out_assign.v",
            "bx_constants_sub_tiehi_sibling.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "missing observed output expression",
        netlists = [
            "bx_constants_alias_out.v",
            "bx_constants_assign_busout_top.v",
            "bx_constants_assign_out_top.v",
            "bx_constants_concat_out_assign.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "no aligned observed outputs (no-driver cones)",
        netlists = [
            "bx_constants_assign_out_sub.v",
            "bx_constants_assign_partsel.v",
            "bx_constants_assign_wire0.v",
            "bx_constants_assign_wire_fanout.v",
            "bx_constants_bus_allones.v",
            "bx_constants_bus_allzero.v",
            "bx_constants_bus_bin.v",
            "bx_constants_bus_dec.v",
            "bx_constants_bus_hex.v",
            "bx_constants_bus_hex_upper.v",
            "bx_constants_bus_out_sub_const.v",
            "bx_constants_bus_wide8.v",
            "bx_constants_chain_assign_const.v",
            "bx_constants_concat_allconst.v",
            "bx_constants_concat_d2.v",
            "bx_constants_concat_mixed.v",
            "bx_constants_const_two_subports.v",
            "bx_constants_feedthrough3.v",
            "bx_constants_feedthrough3_tie1.v",
            "bx_constants_literal_same.v",
            "bx_constants_mux_sel_const.v",
            "bx_constants_repeat_literal_fanout.v",
            "bx_constants_signed_lit.v",
            "bx_constants_tie0_d1.v",
            "bx_constants_tie0_d2.v",
            "bx_constants_tie0_d2_twice.v",
            "bx_constants_tie0_d3.v",
            "bx_constants_tie0_depth1.v",
            "bx_constants_tie0_depth2.v",
            "bx_constants_tie0_depth3.v",
            "bx_constants_tie1_d1.v",
            "bx_constants_tie1_d2.v",
            "bx_constants_tie1_d3.v",
            "bx_constants_tie1_depth1.v",
            "bx_constants_tie1_depth2.v",
            "bx_constants_tie1_depth3.v",
            "bx_constants_tieboth_gate.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "no aligned observed outputs (no-driver cones)",
        netlists = [
            "bx_constants_assign_out_sub.v",
            "bx_constants_assign_partsel.v",
            "bx_constants_assign_wire0.v",
            "bx_constants_assign_wire_fanout.v",
            "bx_constants_bus_allones.v",
            "bx_constants_bus_allzero.v",
            "bx_constants_bus_bin.v",
            "bx_constants_bus_dec.v",
            "bx_constants_bus_hex.v",
            "bx_constants_bus_hex_upper.v",
            "bx_constants_bus_out_sub_const.v",
            "bx_constants_bus_wide8.v",
            "bx_constants_chain_assign_const.v",
            "bx_constants_concat_allconst.v",
            "bx_constants_concat_d2.v",
            "bx_constants_concat_mixed.v",
            "bx_constants_const_two_subports.v",
            "bx_constants_feedthrough3.v",
            "bx_constants_feedthrough3_tie1.v",
            "bx_constants_literal_same.v",
            "bx_constants_mux_sel_const.v",
            "bx_constants_repeat_literal_fanout.v",
            "bx_constants_signed_lit.v",
            "bx_constants_tie0_d1.v",
            "bx_constants_tie0_d2.v",
            "bx_constants_tie0_d2_twice.v",
            "bx_constants_tie0_d3.v",
            "bx_constants_tie0_depth1.v",
            "bx_constants_tie0_depth2.v",
            "bx_constants_tie0_depth3.v",
            "bx_constants_tie1_d1.v",
            "bx_constants_tie1_d2.v",
            "bx_constants_tie1_d3.v",
            "bx_constants_tie1_depth1.v",
            "bx_constants_tie1_depth2.v",
            "bx_constants_tie1_depth3.v",
            "bx_constants_tieboth_gate.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "no-aligned-outputs-remain",
        netlists = [
            "bx_bus_geometry_negonly_child.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "boundary-mismatch",
        symptom = "rhs=[t[0], y[0]]",
        netlists = [
            "wb_sta_reader_supply_port_modifier.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "boundary-mismatch",
        symptom = "rhs=[t[0], y[0]]",
        netlists = [
            "wb_sta_reader_supply_port_modifier.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "syntax error, unexpected CONSTVAL_TK",
        netlists = [
            "wb_writer_digit_module.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "syntax error, unexpected CONSTVAL_TK, expecting ID",
        netlists = [
            "wb_writer_alldigit_net.v",
            "wb_writer_digit_bus_net.v",
            "wb_writer_digit_inst.v",
            "wb_writer_digit_net.v",
            "wb_writer_digit_topmodule.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "syntax error, unexpected CONSTVAL_TK, expecting ID",
        netlists = [
            "wb_writer_alldigit_net.v",
            "wb_writer_digit_bus_net.v",
            "wb_writer_digit_inst.v",
            "wb_writer_digit_inst_in_sub.v",
            "wb_writer_digit_net.v",
            "wb_writer_digit_topmodule.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "syntax error, unexpected CONSTVAL_TK, expecting IN",
        netlists = [
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_port.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "syntax error, unexpected CONSTVAL_TK, expecting IN",
        netlists = [
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_port.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "wire collision for net a/b/n (out_flat.v line 11)",
        netlists = [
            "bx_collisions_net_deep3.v",
            "bx_collisions_synth_net.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "wire collision for net m/c/n (out_flat.v line 11)",
        netlists = [
            "bx_collisions_subnet_esc_vs_path.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "wire collision for net x/p (out_flat.v line 11)",
        netlists = [
            "bx_collisions_unconn_port_vs_esc.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "wire collision for net x/q (out_flat.v line 13)",
        netlists = [
            "bx_collisions_dff_net_vs_esc.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "wire collision for net x/y (out_flat.v line 11)",
        netlists = [
            "bx_collisions_net_vs_flatnet.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "flat bus regroup declares its base name over an existing escaped net",
        netlists = [
            "s2_collisions_busbase_vs_escbus.v",
            "s2_collisions_busbase_vs_escnet.v",
            "s2_collisions_topport_vs_busbase.v",
            "s2_escaping_escbus_vs_flatbus_same_width.v",
            "s2_escaping_escslash_vs_flatbus_d2.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "flat hierarchy path join collides with an escaped user name",
        netlists = [
            "s2_collisions_implicit_net_vs_path.v",
            "s2_collisions_path_depth4_inst.v",
            "s2_collisions_path_depth4_net.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "two synthesized flat names collide with each other",
        netlists = [
            "s2_collisions_dangling_out_two_paths.v",
            "s2_collisions_synth3_way_inst.v",
            "s2_collisions_synth_net_split4.v",
        ],
    ),
    # One defect, two ways out: when the hijacked _NC name still leaves an
    # output reachable the checker proves what it can and skips the rest, and
    # when it does not the checker cannot run at all. Split because the mode is
    # part of what each case pins down.
    xfail(
        path = "hier",
        mode = "partial",
        symptom = "_NC filler wire takes a name the netlist already uses",
        netlists = [
            "s2_collisions_nc_capture_observable.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "_NC filler wire takes a name the netlist already uses",
        netlists = [
            "s2_collisions_nc_two_digit.v",
        ],
    ),
    xfail(
        path = "flat",
        mode = "tool-error",
        symptom = "a keyword-named bus is emitted without the escape it needs",
        netlists = [
            "s2_escaping_kw_bus_net_d1.v",
        ],
    ),
    xfail(
        path = "hier",
        mode = "tool-error",
        symptom = "a keyword-named bus is emitted without the escape it needs",
        netlists = [
            "s2_escaping_kw_bus_net_d1.v",
            "s2_escaping_kw_bus_port_d2.v",
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
        symptom = "read_verilog/link_design rejects the input netlist (ORD-2013)",
        netlists = [
            "structural/wb_sta_reader_blackbox_bus_bit_order.v",
            "structural/wb_sta_reader_blackbox_ordered_ports.v",
            "wb_dbsta_link_unresolved_module.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        symptom = "read_verilog/link_design rejects the input netlist (STA-0171)",
        netlists = [
            "bx_bus_geometry_concat_replicate.v",
            "bx_bus_geometry_replicate_assign.v",
            "bx_constants_repl_concat.v",
            "bx_constants_repl_signal.v",
            "bx_constants_repl_simple.v",
            "bx_dangling_positional_all_empty.v",
            "bx_dangling_positional_hole_leaf.v",
            "bx_dangling_subin_positional_hole.v",
            "bx_dangling_subin_positional_trailing.v",
            "bx_dangling_subout_positional_hole.v",
            "bx_port_rewiring_concat_replication_port_conn.v",
            "bx_port_rewiring_instance_array_bus_split.v",
            "bx_port_rewiring_positional_gap_first.v",
            "bx_port_rewiring_positional_gap_input.v",
            "bx_port_rewiring_positional_gap_last.v",
            "bx_port_rewiring_positional_gap_output.v",
            "structural/wb_sta_reader_empty_specify.v",
            "structural/wb_sta_reader_tri0_net_type.v",
            "wb_sta_reader_attr_before_assign.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        symptom = "read_verilog/link_design rejects the input netlist (STA-1390)",
        netlists = [
            "structural/wb_dbsta_link_attr_impl_oper_unused.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        symptom = "read_verilog/link_design rejects the input netlist by throwing with no OpenROAD error code",
        netlists = [
            "bx_constants_unsized_b0.v",
            "bx_constants_unsized_d1.v",
            "structural/wb_sta_reader_const_negative_width.v",
            "wb_dbsta_link_attr_dont_touch_string.v",
            "wb_sta_reader_attr_dont_touch_string.v",
            "wb_sta_reader_attr_src_line_overflow.v",
            "wb_sta_reader_attr_survival.v",
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
        symptom = "reader rejects an uppercase X digit (STA-0171) though lowercase x is accepted silently",
        netlists = [
            "structural/wb_sta_reader_const_upper_x_digit.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "round_trip",
        symptom = "the emitted netlist cannot be read back (STA-0171)",
        netlists = [
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
            "bx_naming_escaped_inst_kw_assign.v",
            "bx_naming_escaped_net_kw_module.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "bx_naming_escaped_top_kw.v",
            "s2_escaping_kw_bus_net_d1.v",
            "structural/wb_writer_wire_index_overflow.v",
            "wb_writer_alldigit_net.v",
            "wb_writer_digit_bus_net.v",
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_inst.v",
            "wb_writer_digit_net.v",
            "wb_writer_digit_port.v",
            "wb_writer_digit_topmodule.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        symptom = "read_verilog/link_design rejects the input netlist (ORD-2013)",
        netlists = [
            "structural/wb_sta_reader_blackbox_bus_bit_order.v",
            "structural/wb_sta_reader_blackbox_ordered_ports.v",
            "wb_dbsta_link_unresolved_module.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        symptom = "read_verilog/link_design rejects the input netlist (STA-0171)",
        netlists = [
            "bx_bus_geometry_concat_replicate.v",
            "bx_bus_geometry_replicate_assign.v",
            "bx_constants_repl_concat.v",
            "bx_constants_repl_signal.v",
            "bx_constants_repl_simple.v",
            "bx_dangling_positional_all_empty.v",
            "bx_dangling_positional_hole_leaf.v",
            "bx_dangling_subin_positional_hole.v",
            "bx_dangling_subin_positional_trailing.v",
            "bx_dangling_subout_positional_hole.v",
            "bx_port_rewiring_concat_replication_port_conn.v",
            "bx_port_rewiring_instance_array_bus_split.v",
            "bx_port_rewiring_positional_gap_first.v",
            "bx_port_rewiring_positional_gap_input.v",
            "bx_port_rewiring_positional_gap_last.v",
            "bx_port_rewiring_positional_gap_output.v",
            "structural/wb_sta_reader_empty_specify.v",
            "structural/wb_sta_reader_tri0_net_type.v",
            "wb_sta_reader_attr_before_assign.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        symptom = "read_verilog/link_design rejects the input netlist (STA-1390)",
        netlists = [
            "structural/wb_dbsta_link_attr_impl_oper_unused.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        symptom = "read_verilog/link_design rejects the input netlist by throwing with no OpenROAD error code",
        netlists = [
            "bx_constants_unsized_b0.v",
            "bx_constants_unsized_d1.v",
            "structural/wb_sta_reader_const_negative_width.v",
            "wb_dbsta_link_attr_dont_touch_string.v",
            "wb_sta_reader_attr_dont_touch_string.v",
            "wb_sta_reader_attr_src_line_overflow.v",
            "wb_sta_reader_attr_survival.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        symptom = "reader rejects an uppercase X digit (STA-0171) though lowercase x is accepted silently",
        netlists = [
            "structural/wb_sta_reader_const_upper_x_digit.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "round_trip",
        symptom = "the emitted netlist cannot be read back (STA-0171)",
        netlists = [
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
            "bx_naming_escaped_d2_inst_kw.v",
            "bx_naming_escaped_d2_port_kw.v",
            "bx_naming_escaped_inst_kw_assign.v",
            "bx_naming_escaped_mod_kw.v",
            "bx_naming_escaped_net_kw_module.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "bx_naming_escaped_top_kw.v",
            "s2_escaping_kw_bus_net_d1.v",
            "s2_escaping_kw_bus_port_d2.v",
            "structural/wb_writer_wire_index_overflow.v",
            "wb_writer_alldigit_net.v",
            "wb_writer_digit_bus_net.v",
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_inst.v",
            "wb_writer_digit_inst_in_sub.v",
            "wb_writer_digit_module.v",
            "wb_writer_digit_net.v",
            "wb_writer_digit_port.v",
            "wb_writer_digit_topmodule.v",
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
        symptom = "a module definition is dropped",
        netlists = [
            "structural/bx_dangling_module_never_instantiated.v",
            "wb_sta_reader_module_shadows_cell_after.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "module_set",
        symptom = "hier uniquification clones a module per instance",
        netlists = [
            "busslice_two_insts.v",
            "bx_bus_geometry_psel_overlap.v",
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
            "bx_constants_const_two_subports.v",
            "bx_constants_logic_fanout_boundary.v",
            "bx_constants_tie0_d2_twice.v",
            "bx_dangling_asym_two_inst.v",
            "bx_dangling_busport_two_inst_fillers.v",
            "bx_dangling_two_inst_control.v",
            "bx_dangling_undriven_two_inst.v",
            "bx_naming_plain_case_insts_d3.v",
            "bx_port_rewiring_concat_of_two_children.v",
            "bx_port_rewiring_partselect_halves_crossed.v",
            "bx_port_rewiring_perm8_crossed_halves_d2.v",
            "bx_port_rewiring_perm_master_at_two_depths.v",
            "bx_port_rewiring_perm_two_instances_opposite.v",
            "bx_port_rewiring_positional_and_named_same_master.v",
            "bx_port_rewiring_positional_named_scrambled_6ports.v",
            "bx_port_rewiring_same_master_input_unconnected.v",
            "bx_port_rewiring_same_master_output_unconnected.v",
            "bx_port_rewiring_samenet_diff_instances.v",
            "bx_port_rewiring_samenet_in_and_child_out_pair.v",
            "bx_port_rewiring_sibling_chain_bitselect_cross.v",
            "bx_port_rewiring_sibling_chain_bus_slice_cross.v",
            "bx_port_rewiring_sibling_chain_three.v",
            "bx_port_rewiring_swap_symmetric_bus.v",
            "bx_port_rewiring_swap_symmetric_deep.v",
            "bx_port_rewiring_swap_symmetric_positional.v",
            "bx_port_rewiring_swap_symmetric_scalar.v",
            "bx_sequential_ff_thru_feedthru_busslice.v",
            "bx_sequential_ff_thru_feedthru_scalar.v",
            "bx_sequential_sdff_scan_toplevel.v",
            "bx_sequential_shift8_4mods_dffr.v",
            "bx_topology_bintree_d3.v",
            "bx_topology_bintree_d4.v",
            "bx_topology_diamond.v",
            "bx_topology_diamond_asym.v",
            "bx_topology_diamond_d2.v",
            "bx_topology_fanout16.v",
            "bx_topology_fanout16_split.v",
            "bx_topology_prefix_trap_deep.v",
            "bx_topology_prefix_trap_full.v",
            "bx_topology_prefix_trap_many.v",
            "bx_topology_prefix_trap_min.v",
            "bx_topology_reuse_3depths.v",
            "bx_topology_reuse_d1_d3.v",
            "bx_topology_reuse_mixed.v",
            "bx_topology_reuse_series.v",
            "bx_topology_reuse_top_sub.v",
            "bx_topology_siblings_same_inst.v",
            "bx_topology_uniq_cell_collision.v",
            "bx_topology_uniq_scheme_collision.v",
            "bx_topology_uniq_scheme_collision2.v",
            "bx_topology_wide_deep.v",
            "escaped_hier_names.v",
            "fanout_two_subs.v",
            "four_subs_chained.v",
            "inherited/TestReadVerilog_DeepDescendantModBTermCollision.v",
            "inherited/modnet_port_alias.v",
            "s2_collisions_bus_regroup_two_insts.v",
            "s2_collisions_uniq_clone_near_miss.v",
            "s2_portwiring_inout_bus_bit_split_d2.v",
            "s2_portwiring_master_reused_three_depths.v",
            "s2_portwiring_nc_counter_two_modules.v",
            "s2_portwiring_outbus_interleaved_two_insts.v",
            "s2_portwiring_partsel_straddles_split_sources.v",
            "s2_portwiring_three_insts_rotated_slices.v",
            "structural/bx_sequential_shift8_4mods.v",
            "two_subs_chained.v",
            "uniquified_module_collision.v",
            "wb_dbsta_link_attr_impl_oper_stolen_name.v",
            "wb_dbsta_link_uniq_clone_base_ambiguity.v",
            "wb_dbsta_link_uniq_clone_eq_top_name.v",
            "wb_dbsta_link_uniq_clone_eq_top_name2.v",
            "wb_dbsta_link_uniq_fallback_slot_taken.v",
            "wb_writer_nc_crossmodule.v",
            "wb_writer_nc_drift_captures_user_net.v",
            "wb_writer_nc_drift_three.v",
            "wb_writer_nc_same_module_two.v",
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
            "bx_bus_geometry_bit_local_thru.v",
            "bx_bus_geometry_bit_two_ports.v",
            "bx_bus_geometry_concat_all_const.v",
            "bx_bus_geometry_const_gatepin_top.v",
            "bx_bus_geometry_const_scalar_port.v",
            "bx_bus_geometry_psel_overlap.v",
            "bx_bus_geometry_scalar_busbit_d3.v",
            "bx_collisions_bracket_underscore_alias.v",
            "bx_collisions_bracket_underscore_suffix.v",
            "bx_collisions_dff_net_vs_esc.v",
            "bx_collisions_escaped_bracket_form.v",
            "bx_collisions_port_in_vs_flatnet.v",
            "bx_collisions_topport_escaped.v",
            "bx_constants_alias_out.v",
            "bx_constants_assign_busout_top.v",
            "bx_constants_concat_mixed.v",
            "bx_constants_logic1_outport.v",
            "bx_constants_port_named_zero.v",
            "bx_dangling_dff_dead.v",
            "bx_dangling_dff_qn_open.v",
            "bx_dangling_nc_collision_outport.v",
            "bx_dangling_nc_collision_port.v",
            "bx_dangling_portorder_control.v",
            "bx_dangling_subout_open_dff.v",
            "bx_dangling_topin_bit_unused.v",
            "bx_dangling_topin_unused.v",
            "bx_dangling_topin_unused_bus.v",
            "bx_dangling_topin_unused_dollar.v",
            "bx_dangling_topin_unused_escaped.v",
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
            "bx_port_rewiring_leaf_positional_dff.v",
            "bx_port_rewiring_nonansi_top_header_output_first.v",
            "bx_port_rewiring_sibling_fanout_three_ways.v",
            "bx_sequential_ck_logic0_dffr.v",
            "bx_sequential_ck_two_ports_same_net.v",
            "bx_sequential_clk_as_data_xor.v",
            "bx_sequential_clk_clkbuf_mid.v",
            "bx_sequential_clk_feedthru_d2.v",
            "bx_sequential_clk_feedthru_d3.v",
            "bx_sequential_clk_inv_in_sub.v",
            "bx_sequential_clk_upward.v",
            "bx_sequential_dffr_reset_feedthru_d3.v",
            "bx_sequential_dffr_rn_from_ff_qn.v",
            "bx_sequential_esc_clknet_collide.v",
            "bx_sequential_esc_clknet_collide_d2.v",
            "bx_sequential_esc_qnet_collide.v",
            "bx_sequential_fb_cross_boundary.v",
            "bx_sequential_fb_cross_two_levels.v",
            "bx_sequential_fb_mux_hold_sub.v",
            "bx_sequential_fb_selfloop_sameport.v",
            "bx_sequential_fb_toggle_inv_sub.v",
            "bx_sequential_ff_bus_bank_reversed.v",
            "bx_sequential_ff_bus_bank_split.v",
            "bx_sequential_ff_busslice_to_topout.v",
            "bx_sequential_ff_esc_inst_name.v",
            "bx_sequential_ff_thru_feedthru_busslice.v",
            "bx_sequential_ff_thru_feedthru_scalar.v",
            "bx_sequential_gen_clk_and_cross.v",
            "bx_sequential_icg_deep_l3.v",
            "bx_sequential_icg_shared_two_subs.v",
            "bx_sequential_probe_clkgate.v",
            "bx_sequential_probe_dff_base.v",
            "bx_sequential_probe_dffr.v",
            "bx_sequential_probe_dffrs.v",
            "bx_sequential_probe_dffs.v",
            "bx_sequential_q_qn_split_subs.v",
            "bx_sequential_q_top_and_cone.v",
            "bx_sequential_qn_direct_top_port_d2.v",
            "bx_sequential_qn_only_dangling_q.v",
            "bx_sequential_qn_only_deep_d3.v",
            "bx_sequential_ripple_clk_cross.v",
            "bx_sequential_sdff_scan_toplevel.v",
            "bx_sequential_shift8_4mods_dffr.v",
            "bx_sequential_shift8_nest4_dffr.v",
            "bx_sequential_two_clk_gated_and.v",
            "bx_sequential_two_clk_gated_icg.v",
            "bx_sequential_two_clk_two_subs_dffr.v",
            "bx_topology_bintree_d3.v",
            "bx_topology_bintree_d4.v",
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
            "s2_collisions_bracket_triple_alias.v",
            "s2_collisions_nc_capture_observable.v",
            "s2_collisions_topport_vs_busbase.v",
            "s2_escaping_esc_dollar_bus_port_d2.v",
            "s2_escaping_kw_bus_port_d2.v",
            "s2_portwiring_concat_mixed_widths_d2.v",
            "s2_portwiring_concat_three_sources_w5_d2.v",
            "s2_portwiring_named_port_omitted_d3.v",
            "s2_portwiring_positional_bus_midlist_d3.v",
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
            "wb_dbsta_link_open_out_formal_bus_fanout.v",
            "wb_sta_reader_ansi_bus_continuation.v",
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_port.v",
            "wb_writer_escbusbit_vs_realbus.v",
            "wb_writer_hier_bus_mixed_concat.v",
            "wb_writer_portorder_bus_last.v",
            "wb_writer_portorder_netname_key.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "top_ports",
        symptom = "port list reordered and a direction or bus range changed",
        netlists = [
            "structural/wb_sta_reader_header_bitselect_port.v",
            "wb_sta_reader_supply_port_modifier.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "top_ports",
        symptom = "port list reordered and a port lost",
        netlists = [
            "bx_bus_geometry_neg_partsel.v",
            "bx_bus_geometry_neg_top.v",
            "bx_bus_geometry_negonly_top.v",
            "bx_naming_escaped_port_kw_input.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "top_ports",
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
            "bx_bus_geometry_bit_local_thru.v",
            "bx_bus_geometry_bit_two_ports.v",
            "bx_bus_geometry_concat_all_const.v",
            "bx_bus_geometry_const_gatepin_top.v",
            "bx_bus_geometry_const_scalar_port.v",
            "bx_bus_geometry_psel_overlap.v",
            "bx_bus_geometry_scalar_busbit_d3.v",
            "bx_collisions_bracket_underscore_alias.v",
            "bx_collisions_bracket_underscore_suffix.v",
            "bx_collisions_dff_net_vs_esc.v",
            "bx_collisions_escaped_bracket_form.v",
            "bx_collisions_port_in_vs_flatnet.v",
            "bx_collisions_topport_escaped.v",
            "bx_constants_alias_out.v",
            "bx_constants_assign_busout_top.v",
            "bx_constants_concat_mixed.v",
            "bx_constants_logic1_outport.v",
            "bx_constants_port_named_zero.v",
            "bx_dangling_dff_dead.v",
            "bx_dangling_dff_qn_open.v",
            "bx_dangling_nc_collision_outport.v",
            "bx_dangling_nc_collision_port.v",
            "bx_dangling_portorder_control.v",
            "bx_dangling_subout_open_dff.v",
            "bx_dangling_topin_bit_unused.v",
            "bx_dangling_topin_unused.v",
            "bx_dangling_topin_unused_bus.v",
            "bx_dangling_topin_unused_dollar.v",
            "bx_dangling_topin_unused_escaped.v",
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
            "bx_port_rewiring_leaf_positional_dff.v",
            "bx_port_rewiring_nonansi_top_header_output_first.v",
            "bx_port_rewiring_sibling_fanout_three_ways.v",
            "bx_sequential_ck_logic0_dffr.v",
            "bx_sequential_ck_two_ports_same_net.v",
            "bx_sequential_clk_as_data_xor.v",
            "bx_sequential_clk_clkbuf_mid.v",
            "bx_sequential_clk_feedthru_d2.v",
            "bx_sequential_clk_feedthru_d3.v",
            "bx_sequential_clk_inv_in_sub.v",
            "bx_sequential_clk_upward.v",
            "bx_sequential_dffr_reset_feedthru_d3.v",
            "bx_sequential_dffr_rn_from_ff_qn.v",
            "bx_sequential_esc_clknet_collide.v",
            "bx_sequential_esc_clknet_collide_d2.v",
            "bx_sequential_esc_qnet_collide.v",
            "bx_sequential_fb_cross_boundary.v",
            "bx_sequential_fb_cross_two_levels.v",
            "bx_sequential_fb_mux_hold_sub.v",
            "bx_sequential_fb_selfloop_sameport.v",
            "bx_sequential_fb_toggle_inv_sub.v",
            "bx_sequential_ff_bus_bank_reversed.v",
            "bx_sequential_ff_bus_bank_split.v",
            "bx_sequential_ff_busslice_to_topout.v",
            "bx_sequential_ff_esc_inst_name.v",
            "bx_sequential_ff_thru_feedthru_busslice.v",
            "bx_sequential_ff_thru_feedthru_scalar.v",
            "bx_sequential_gen_clk_and_cross.v",
            "bx_sequential_icg_deep_l3.v",
            "bx_sequential_icg_shared_two_subs.v",
            "bx_sequential_probe_clkgate.v",
            "bx_sequential_probe_dff_base.v",
            "bx_sequential_probe_dffr.v",
            "bx_sequential_probe_dffrs.v",
            "bx_sequential_probe_dffs.v",
            "bx_sequential_q_qn_split_subs.v",
            "bx_sequential_q_top_and_cone.v",
            "bx_sequential_qn_direct_top_port_d2.v",
            "bx_sequential_qn_only_dangling_q.v",
            "bx_sequential_qn_only_deep_d3.v",
            "bx_sequential_ripple_clk_cross.v",
            "bx_sequential_sdff_scan_toplevel.v",
            "bx_sequential_shift8_4mods_dffr.v",
            "bx_sequential_shift8_nest4_dffr.v",
            "bx_sequential_two_clk_gated_and.v",
            "bx_sequential_two_clk_gated_icg.v",
            "bx_sequential_two_clk_two_subs_dffr.v",
            "bx_topology_bintree_d3.v",
            "bx_topology_bintree_d4.v",
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
            "s2_collisions_bracket_triple_alias.v",
            "s2_collisions_nc_capture_observable.v",
            "s2_collisions_topport_vs_busbase.v",
            "s2_escaping_esc_dollar_bus_port_d2.v",
            "s2_escaping_kw_bus_port_d2.v",
            "s2_portwiring_concat_mixed_widths_d2.v",
            "s2_portwiring_concat_three_sources_w5_d2.v",
            "s2_portwiring_named_port_omitted_d3.v",
            "s2_portwiring_positional_bus_midlist_d3.v",
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
            "wb_dbsta_link_open_out_formal_bus_fanout.v",
            "wb_sta_reader_ansi_bus_continuation.v",
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_port.v",
            "wb_writer_escbusbit_vs_realbus.v",
            "wb_writer_hier_bus_mixed_concat.v",
            "wb_writer_portorder_bus_last.v",
            "wb_writer_portorder_netname_key.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "top_ports",
        symptom = "port list reordered and a direction or bus range changed",
        netlists = [
            "structural/wb_sta_reader_header_bitselect_port.v",
            "wb_sta_reader_supply_port_modifier.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "top_ports",
        symptom = "port list reordered and a port lost",
        netlists = [
            "bx_bus_geometry_neg_partsel.v",
            "bx_bus_geometry_neg_top.v",
            "bx_bus_geometry_negonly_top.v",
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
        symptom = "a declared net or port is erased",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "bx_bus_geometry_asc_wire_assign.v",
            "bx_bus_geometry_bitsel_w1.v",
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
            "bx_bus_geometry_partsel_nested.v",
            "bx_constants_alias_out.v",
            "bx_constants_assign_partsel.v",
            "bx_constants_assign_wire0.v",
            "bx_constants_assign_wire_fanout.v",
            "bx_constants_bus_out_sub_const.v",
            "bx_constants_chain_assign_const.v",
            "bx_constants_user_one_wire.v",
            "bx_constants_user_zero_wire.v",
            "bx_constants_user_zero_wire_ctl.v",
            "bx_dangling_bus_slice_top.v",
            "bx_dangling_deadassign.v",
            "bx_dangling_deadassign_chain.v",
            "bx_dangling_deadassign_concat.v",
            "bx_dangling_deadassign_const.v",
            "bx_dangling_deadwire_decl.v",
            "bx_dangling_deadwire_escaped.v",
            "bx_dangling_subout_dead_assign.v",
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_port_rewiring_offset_net_range_high.v",
            "bx_port_rewiring_offset_partselect_from_offset_net.v",
            "bx_sequential_ff_thru_feedthru_busslice.v",
            "bx_sequential_ff_thru_feedthru_scalar.v",
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
            "wb_dbnetwork_overlay_child_out_alias_two_ports.v",
            "wb_dbsta_link_supply_net_hier_boundary.v",
            "wb_dbsta_link_supply_net_sigtype.v",
            "wb_sta_reader_supply_tie.v",
            "wb_writer_wire_bus_5to5.v",
            "structural/wb_writer_wire_index_intmax.v",
            "wb_writer_wire_lsbfirst_flip.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "declared_nets",
        symptom = "a declared net or port is erased; a name absent from the input is invented",
        netlists = [
            "bx_bus_geometry_neg_partsel.v",
            "bx_bus_geometry_neg_top.v",
            "bx_bus_geometry_negonly_top.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "declared_nets",
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
        symptom = "wire w carrying the x constant is erased from the output",
        netlists = [
            "structural/bx_constants_assign_x_wire.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        symptom = "_NC filler wires are invented",
        netlists = [
            "bx_dangling_bus_in_empty.v",
            "bx_dangling_busout_port_open.v",
            "bx_dangling_busport_nc2_collision.v",
            "bx_dangling_busport_ncname_collision.v",
            "bx_dangling_busport_two_inst_fillers.v",
            "bx_dangling_busport_whole_empty.v",
            "bx_dangling_busport_whole_omitted.v",
            "bx_dangling_nc_collision_bus.v",
            "bx_dangling_nc_collision_mid.v",
            "bx_dangling_nc_collision_outport.v",
            "bx_dangling_nc_collision_port.v",
            "bx_dangling_nc_collision_scalar_live.v",
            "bx_dangling_nc_two_bus_ports.v",
            "bx_port_rewiring_named_empty_bus_input.v",
            "bx_port_rewiring_named_empty_bus_output.v",
            "s2_collisions_nc_capture_observable.v",
            "s2_collisions_nc_two_digit.v",
            "s2_portwiring_nc_counter_two_modules.v",
            "s2_portwiring_unconn_bus_input_d3.v",
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
            "wb_dbsta_link_open_out_formal_bus_fanout.v",
            "wb_writer_nc_crossmodule.v",
            "wb_writer_nc_drift_captures_user_net.v",
            "wb_writer_nc_drift_three.v",
            "wb_writer_nc_inout_vector.v",
            "wb_writer_nc_same_module_two.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        symptom = "a declared net or port is erased",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "busslice_mid.v",
            "bx_bus_geometry_asc_wire_assign.v",
            "bx_bus_geometry_bitsel_w1.v",
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
            "bx_bus_geometry_partsel_nested.v",
            "bx_bus_geometry_scalar_busbit_d3.v",
            "bx_constants_alias_out.v",
            "bx_constants_assign_partsel.v",
            "bx_constants_assign_wire0.v",
            "bx_constants_assign_wire_fanout.v",
            "bx_constants_chain_assign_const.v",
            "bx_constants_sub_zero_capture.v",
            "bx_constants_user_one_wire.v",
            "bx_constants_user_zero_wire.v",
            "bx_constants_user_zero_wire_ctl.v",
            "bx_dangling_bus_slice_top.v",
            "bx_dangling_deadassign.v",
            "bx_dangling_deadassign_chain.v",
            "bx_dangling_deadassign_concat.v",
            "bx_dangling_deadassign_const.v",
            "bx_dangling_deadassign_in_sub.v",
            "bx_dangling_deadwire_decl.v",
            "bx_dangling_deadwire_escaped.v",
            "bx_dangling_subout_dead_assign.v",
            "bx_dangling_undriven_leak_collide.v",
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_port_rewiring_offset_net_range_high.v",
            "bx_port_rewiring_offset_partselect_from_offset_net.v",
            "bx_topology_feedthrough_assign_d4.v",
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
            "wb_dbnetwork_overlay_depth0_escslash_rename_port.v",
            "wb_dbnetwork_overlay_erase_overshoot_net_victim.v",
            "wb_dbnetwork_overlay_netname_erase_overshoot_port.v",
            "wb_dbnetwork_overlay_netname_header_single_bslash.v",
            "wb_dbnetwork_overlay_stamped_path_dup_decl.v",
            "wb_dbsta_link_alias_name_after_port.v",
            "wb_dbsta_link_alias_name_before_port.v",
            "wb_dbsta_link_supply_net_hier_boundary.v",
            "wb_dbsta_link_supply_net_sigtype.v",
            "wb_sta_reader_supply_tie.v",
            "wb_writer_hier_input_alias_orphan.v",
            "wb_writer_wire_bus_5to5.v",
            "structural/wb_writer_wire_index_intmax.v",
            "wb_writer_wire_lsbfirst_flip.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        symptom = "a declared net or port is erased; _NC filler wires are invented",
        netlists = [
            "structural/wb_sta_reader_bus_dcl_initializer.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        symptom = "a declared net or port is erased; a module-local net is renamed to an instance path",
        netlists = [
            "bx_dangling_gate_island_leaf.v",
            "bx_dangling_positional_in_sub.v",
            "bx_dangling_undriven_bus_in_sub.v",
            "bx_dangling_undriven_depth2_island.v",
            "bx_dangling_undriven_esc_leak.v",
            "bx_dangling_undriven_leak_escinst.v",
            "bx_dangling_undriven_two_inst.v",
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
        symptom = "a declared net or port is erased; a name absent from the input is invented",
        netlists = [
            "bx_bus_geometry_neg_partsel.v",
            "bx_bus_geometry_neg_top.v",
            "bx_bus_geometry_negonly_top.v",
            "structural/bx_port_rewiring_explicit_port_bus_halves_crossed.v",
            "structural/bx_port_rewiring_explicit_port_bus_whole.v",
            "structural/bx_port_rewiring_explicit_port_concat_perm.v",
            "structural/bx_port_rewiring_explicit_port_positional_bus.v",
            "structural/wb_dbnetwork_overlay_depth0_escslash_rename.v",
            "wb_dbnetwork_overlay_dcflat_hier_mixed.v",
            "wb_dbnetwork_overlay_dcflat_pathnames_short.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        symptom = "a name absent from the input is invented",
        netlists = [
            "structural/wb_sta_reader_hier_ref_dotted_id.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
        symptom = "an undeclared constant net is referenced",
        netlists = [
            "bx_constants_assign_out_sub.v",
            "bx_constants_bus_out_sub_const.v",
            "bx_constants_feedthrough3.v",
            "bx_constants_sub_tiehi_sibling.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "declared_nets",
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
        symptom = "instances rebound to a uniquified clone or to another master",
        netlists = [
            "busslice_two_insts.v",
            "bx_bus_geometry_psel_overlap.v",
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
            "bx_constants_const_two_subports.v",
            "bx_constants_logic_fanout_boundary.v",
            "bx_constants_tie0_d2_twice.v",
            "bx_dangling_asym_two_inst.v",
            "bx_dangling_busport_two_inst_fillers.v",
            "bx_dangling_two_inst_control.v",
            "bx_dangling_undriven_two_inst.v",
            "bx_naming_plain_case_insts_d3.v",
            "bx_port_rewiring_concat_of_two_children.v",
            "bx_port_rewiring_partselect_halves_crossed.v",
            "bx_port_rewiring_perm8_crossed_halves_d2.v",
            "bx_port_rewiring_perm_master_at_two_depths.v",
            "bx_port_rewiring_perm_two_instances_opposite.v",
            "bx_port_rewiring_positional_and_named_same_master.v",
            "bx_port_rewiring_positional_named_scrambled_6ports.v",
            "bx_port_rewiring_same_master_input_unconnected.v",
            "bx_port_rewiring_same_master_output_unconnected.v",
            "bx_port_rewiring_samenet_diff_instances.v",
            "bx_port_rewiring_samenet_in_and_child_out_pair.v",
            "bx_port_rewiring_sibling_chain_bitselect_cross.v",
            "bx_port_rewiring_sibling_chain_bus_slice_cross.v",
            "bx_port_rewiring_sibling_chain_three.v",
            "bx_port_rewiring_swap_symmetric_bus.v",
            "bx_port_rewiring_swap_symmetric_deep.v",
            "bx_port_rewiring_swap_symmetric_positional.v",
            "bx_port_rewiring_swap_symmetric_scalar.v",
            "bx_sequential_ff_thru_feedthru_busslice.v",
            "bx_sequential_ff_thru_feedthru_scalar.v",
            "bx_sequential_sdff_scan_toplevel.v",
            "bx_sequential_shift8_4mods_dffr.v",
            "bx_topology_bintree_d3.v",
            "bx_topology_bintree_d4.v",
            "bx_topology_diamond.v",
            "bx_topology_diamond_asym.v",
            "bx_topology_diamond_d2.v",
            "bx_topology_fanout16.v",
            "bx_topology_fanout16_split.v",
            "bx_topology_prefix_trap_deep.v",
            "bx_topology_prefix_trap_full.v",
            "bx_topology_prefix_trap_many.v",
            "bx_topology_prefix_trap_min.v",
            "bx_topology_reuse_3depths.v",
            "bx_topology_reuse_d1_d3.v",
            "bx_topology_reuse_mixed.v",
            "bx_topology_reuse_series.v",
            "bx_topology_reuse_top_sub.v",
            "bx_topology_siblings_same_inst.v",
            "bx_topology_uniq_cell_collision.v",
            "bx_topology_uniq_scheme_collision.v",
            "bx_topology_uniq_scheme_collision2.v",
            "bx_topology_wide_deep.v",
            "escaped_hier_names.v",
            "fanout_two_subs.v",
            "four_subs_chained.v",
            "inherited/TestReadVerilog_DeepDescendantModBTermCollision.v",
            "inherited/modnet_port_alias.v",
            "s2_collisions_bus_regroup_two_insts.v",
            "s2_collisions_uniq_clone_near_miss.v",
            "s2_portwiring_inout_bus_bit_split_d2.v",
            "s2_portwiring_master_reused_three_depths.v",
            "s2_portwiring_nc_counter_two_modules.v",
            "s2_portwiring_outbus_interleaved_two_insts.v",
            "s2_portwiring_partsel_straddles_split_sources.v",
            "s2_portwiring_three_insts_rotated_slices.v",
            "structural/bx_collisions_escaped_equals_plain_inst.v",
            "structural/bx_collisions_uniq_vs_module_uninst.v",
            "structural/bx_sequential_shift8_4mods.v",
            "structural/wb_dbnetwork_overlay_strip_bslash_inst_dup.v",
            "structural/wb_dbnetwork_overlay_strip_parent_trailing_bslash.v",
            "structural/wb_sta_reader_ifdef_body_compiled.v",
            "two_subs_chained.v",
            "uniquified_module_collision.v",
            "wb_dbsta_link_attr_impl_oper_stolen_name.v",
            "wb_dbsta_link_uniq_clone_base_ambiguity.v",
            "wb_dbsta_link_uniq_clone_eq_top_name.v",
            "wb_dbsta_link_uniq_clone_eq_top_name2.v",
            "wb_dbsta_link_uniq_fallback_slot_taken.v",
            "wb_writer_nc_crossmodule.v",
            "wb_writer_nc_drift_captures_user_net.v",
            "wb_writer_nc_drift_three.v",
            "wb_writer_nc_same_module_two.v",
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
        symptom = "a clone takes an existing module name, so the name denotes the wrong module",
        netlists = [
            "bx_collisions_uniq_chain.v",
            "bx_collisions_uniq_theft_order_z.v",
            "bx_collisions_uniq_theft_victim_below.v",
            "bx_collisions_uniq_vs_module_collide.v",
            "bx_collisions_uniq_vs_module_collide_rev.v",
            "bx_collisions_uniq_vs_module_same.v",
            "bx_topology_uniq_scheme_collision.v",
            "bx_topology_uniq_scheme_collision2.v",
            "structural/bx_collisions_uniq_vs_module_uninst.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "name_identity",
        symptom = "one clone name is the uniquification name of two different modules",
        netlists = [
            "bx_collisions_uniq_cross_prefix.v",
            "wb_dbsta_link_uniq_clone_base_ambiguity.v",
        ],
    ),

    # ----------------------------------------------------------------------
    # cell_census
    #
    # The number of elaborated leaf instances changed -- gates were gained or
    # lost outright.
    structural_xfail(
        path = "flat",
        check = "cell_census",
        symptom = "the elaborated leaf instance count changed",
        netlists = [
            "wb_sta_reader_module_shadows_cell_after.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "cell_census",
        symptom = "the elaborated leaf instance count changed",
        netlists = [
            "bx_collisions_uniq_clone_eq_libcell.v",
            "wb_sta_reader_module_shadows_cell_after.v",
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
        symptom = "a continuous assign is dropped",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "bx_bus_geometry_asc_wire_assign.v",
            "bx_bus_geometry_bitsel_w1.v",
            "bx_bus_geometry_partsel_nested.v",
            "bx_constants_alias_out.v",
            "bx_constants_assign_busout_top.v",
            "bx_constants_assign_out_bitsel.v",
            "bx_constants_assign_out_top.v",
            "bx_constants_assign_partsel.v",
            "bx_constants_assign_wire0.v",
            "bx_constants_assign_wire_fanout.v",
            "bx_constants_chain_assign_const.v",
            "bx_dangling_bus_slice_top.v",
            "bx_dangling_deadassign.v",
            "bx_dangling_deadassign_chain.v",
            "bx_dangling_deadassign_concat.v",
            "bx_dangling_deadassign_const.v",
            "bx_dangling_subout_dead_assign.v",
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
        symptom = "the assign driving the x constant is dropped with its wire",
        netlists = [
            "structural/bx_constants_assign_x_wire.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "assigns",
        symptom = "a continuous assign is dropped",
        netlists = [
            "alias_both_used.v",
            "alias_both_used_bus.v",
            "alias_net_on_port.v",
            "busslice_mid.v",
            "bx_bus_geometry_asc_wire_assign.v",
            "bx_bus_geometry_bitsel_w1.v",
            "bx_bus_geometry_partsel_nested.v",
            "bx_bus_geometry_scalar_busbit_d3.v",
            "bx_constants_alias_out.v",
            "bx_constants_assign_busout_top.v",
            "bx_constants_assign_out_bitsel.v",
            "bx_constants_assign_out_top.v",
            "bx_constants_assign_partsel.v",
            "bx_constants_assign_wire0.v",
            "bx_constants_assign_wire_fanout.v",
            "bx_constants_chain_assign_const.v",
            "bx_dangling_bus_slice_top.v",
            "bx_dangling_deadassign.v",
            "bx_dangling_deadassign_chain.v",
            "bx_dangling_deadassign_concat.v",
            "bx_dangling_deadassign_const.v",
            "bx_dangling_deadassign_in_sub.v",
            "bx_dangling_subout_dead_assign.v",
            "bx_topology_feedthrough_assign_d4.v",
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
            "wb_dbsta_link_alias_name_after_port.v",
            "wb_dbsta_link_alias_name_before_port.v",
            "wb_writer_hier_input_alias_orphan.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "assigns",
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
            "bx_sequential_esc_clknet_collide.v",
            "bx_sequential_esc_clknet_collide_d2.v",
            "bx_sequential_esc_qnet_collide.v",
            "s2_collisions_busbase_vs_escbus.v",
            "s2_collisions_busbase_vs_escnet.v",
            "s2_collisions_dangling_out_two_paths.v",
            "s2_collisions_implicit_net_vs_path.v",
            "s2_collisions_path_depth4_net.v",
            "s2_collisions_synth_net_split4.v",
            "s2_escaping_escbus_vs_flatbus_same_width.v",
            "s2_escaping_escslash_vs_flatbus_d2.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "namespace",
        symptom = "an identifier is emitted without the escape it needs",
        netlists = [
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "structural/wb_writer_wire_index_overflow.v",
            "wb_writer_alldigit_net.v",
            "wb_writer_digit_bus_net.v",
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_inst.v",
            "wb_writer_digit_net.v",
            "wb_writer_digit_port.v",
        ],
    ),
    structural_xfail(
        path = "flat",
        check = "namespace",
        symptom = "one name is used for a net and an instance",
        netlists = [
            "bx_collisions_inst_vs_net_cross.v",
            "bx_collisions_net_and_inst_same_name.v",
            "bx_collisions_net_vs_inst_cross.v",
            "bx_collisions_outport_vs_instpath.v",
            "bx_collisions_topport_escaped.v",
            "bx_naming_escaped_slashcol_instnet.v",
            "s2_collisions_synth_inst_vs_net.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "namespace",
        symptom = "a name is declared twice in one module",
        netlists = [
            "bx_dangling_busport_nc2_collision.v",
            "bx_dangling_busport_ncname_collision.v",
            "bx_dangling_nc_collision_bus.v",
            "bx_dangling_nc_collision_mid.v",
            "bx_dangling_nc_collision_scalar_live.v",
            "bx_dangling_nc_filler_all4_taken.v",
            "bx_dangling_undriven_leak_collide.v",
            "s2_collisions_nc_two_digit.v",
            "wb_dbnetwork_overlay_dcflat_hier_mixed.v",
            "wb_dbnetwork_overlay_dcflat_pathnames_short.v",
            "wb_dbnetwork_overlay_erase_overshoot_net_victim.v",
            "wb_dbnetwork_overlay_netname_header_single_bslash.v",
            "wb_dbnetwork_overlay_stamped_path_dup_decl.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "namespace",
        symptom = "an identifier is emitted without the escape it needs",
        netlists = [
            "bx_bus_geometry_negwire_allneg.v",
            "bx_bus_geometry_negwire_desc.v",
            "bx_bus_geometry_negwire_top.v",
            "bx_naming_escaped_d2_port_kw.v",
            "bx_naming_escaped_net_kw_logic.v",
            "bx_naming_escaped_net_kw_wire.v",
            "bx_naming_escaped_port_kw_input.v",
            "structural/wb_writer_wire_index_overflow.v",
            "wb_writer_alldigit_net.v",
            "wb_writer_digit_bus_net.v",
            "wb_writer_digit_bus_port.v",
            "wb_writer_digit_inst.v",
            "wb_writer_digit_inst_in_sub.v",
            "wb_writer_digit_net.v",
            "wb_writer_digit_port.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "namespace",
        symptom = "one name is used for a net and an instance",
        netlists = [
            "bx_collisions_net_and_inst_same_name.v",
        ],
    ),
    structural_xfail(
        path = "hier",
        check = "submodule_ports",
        symptom = "port list reordered",
        netlists = [
            "s2_escaping_kw_bus_port_d2.v",
        ],
    ),
]
