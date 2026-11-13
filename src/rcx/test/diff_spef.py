# Compare native SPEF data without importing it into the extracted database.
import hashlib
import json
import math
import os
import re
from collections import Counter
from pathlib import Path

import helpers
import odb
import rcx
from openroad import Tech

HERE = Path(__file__).absolute().parent
TEST_NETS = "3 48 92 193 200 243 400 521 671"


def snapshot(block):
    corners = range(block.getCornerCount())
    return {
        "corners": (
            block.getCornerCount(),
            block.getCornersPerBlock(),
            block.getCornerNameList(),
        ),
        "nets": [
            (
                n.getId(),
                n.getName(),
                n.isSpef(),
                n.isMarked(),
                n.isMark_1ed(),
                n.isExtracted(),
                n.get1stCapNodeId(),
                n.get1stRSegId(),
                tuple(c.getId() for c in n.getCapNodes()),
                tuple(r.getId() for r in n.getRSegs()),
            )
            for n in block.getNets()
        ],
        "capnodes": [
            (
                c.getId(),
                c.getNet().getId(),
                c.getNode(),
                c.isInternal(),
                c.isBTerm(),
                c.isITerm(),
                tuple(c.getCapacitance(k) for k in corners),
            )
            for c in block.getCapNodes()
        ],
        "rsegs": [
            (
                r.getId(),
                r.getNet().getId(),
                r.getSourceNode(),
                r.getTargetNode(),
                tuple(r.getResistance(k) for k in corners),
                tuple(r.getCapacitance(k, 1.0) for k in corners),
            )
            for r in block.getRSegs()
        ],
        "ccsegs": [
            (
                c.getId(),
                c.getSourceCapNode().getId(),
                c.getTargetCapNode().getId(),
                tuple(c.getCapacitance(k) for k in corners),
            )
            for c in block.getCCSegs()
        ],
        "bterms": [(t.getId(), t.getExtId()) for t in block.getBTerms()],
        "iterms": [(t.getId(), t.getExtId()) for t in block.getITerms()],
        "insts": [(i.getId(), i.getUserFlag1()) for i in block.getInsts()],
    }


def summary(state):
    return {
        "sha256": hashlib.sha256(json.dumps(state).encode()).hexdigest(),
        "counts": {k: len(v) for k, v in state.items()},
    }


def metrics(path):
    result = {}
    couplings = {}
    kinds = {"netCap", "netGndCap", "netCcap", "netRes"}
    for line in path.read_text().splitlines():
        fields = line.split()
        if "ccCap" in fields:
            nodes_index = fields.index("capNodes") + 1
            key = (
                int(fields[fields.index("corner") + 1]),
                *(int(n) for n in fields[nodes_index : nodes_index + 2]),
            )
            assert key not in couplings, ("duplicate coupling comparison", key)
            couplings[key] = (
                float(fields[0]),
                float(fields[fields.index("ccCap") + 1]),
                float(fields[fields.index("ref") + 1]),
            )
        kind = next((f for f in fields if f in kinds), None)
        if kind is not None:
            key = (kind, int(fields[fields.index("corner") + 1]), int(fields[-2]))
            assert key not in result, ("duplicate comparison", key)
            result[key] = (
                float(fields[0]),
                float(fields[1]),
                float(fields[fields.index("ref") + 1]),
            )
    assert all(
        math.isfinite(v) for row in [*result.values(), *couplings.values()] for v in row
    )
    return result, couplings


tech = Tech()
tech.readLef(str(HERE / "sky130hs/sky130hs.tlef"))
design = helpers.make_design(tech)
design.readDef(str(HERE / "generate_pattern.defok"))
extractor = design.getOpenRCX()
extractor.define_process_corner(0, "X")
extractor.define_derived_corner("scaled", "X", 1.2, 1.3, 1.4)
extractor.setExtractionRulesFile(str(HERE / "ext_pattern.rules"))
extract = rcx.ExtractOptions()
extract.cc_model = 12
extract.max_res = 0
extract.context_depth = 10
extract.coupling_threshold = 0.1
extract._version = 1.0
extract.over_cell = True
extractor.extract(extract)
block = design.getBlock()
assert block.getCornerCount() == 2
net = odb.dbNet.getNet(block, 3)


def add_segment(source_node, target_node, resistance, capacitance):
    zero_segment = net.getZeroRSeg()
    first_segment = next(iter(net.getRSegs()))
    added = odb.dbRSeg.create(net, 0, 0, 0, True)
    net.set1stRSegId(zero_segment.getId())
    zero_segment.setNext(added.getId())
    added.setNext(first_segment.getId())
    added.setSourceNode(source_node)
    added.setTargetNode(target_node)
    for corner in range(block.getCornerCount()):
        added.setResistance(resistance + corner, corner)
        added.setCapacitance(capacitance + corner, corner)
    return added


# An internal node whose number must not be collapsed by comparison. Its
# scaled-corner values deliberately differ from automatic corner scaling.
internal = odb.dbCapNode.create(net, 97, False)
internal.setInternalFlag()
segment = add_segment(net.getZeroRSeg().getTargetNode(), internal.getId(), 7.0, 0.5)
assert internal.getNode() == 97

# Exercise real ITerm lookup and coupling values, not just aggregate BTerm RC.
database = tech.getDB()
library = odb.dbLib.create(database, "diff_lib", database.getTech(), "/")
master = odb.dbMaster.create(library, "diff_cell")
master.setWidth(1000)
master.setHeight(1000)
master.setType("CORE")
odb.dbMTerm.create(master, "A", "INPUT")
master.setFrozen()
instances = []
iterms = []
iterm_cap_ids = []
iterm_segments = []
iterm_couplings = []
other_cap = next(iter(odb.dbNet.getNet(block, 4).getCapNodes()))
for index in range(2):
    instance = odb.dbInst.create(block, master, "diff_inst_" + str(index))
    iterm = instance.findITerm("A")
    iterm.connect(net)
    pin_cap = odb.dbCapNode.create(net, iterm.getId(), False)
    pin_cap.setITermFlag()
    iterm_segments.append(
        add_segment(internal.getId(), pin_cap.getId(), 11.0 + index, 0.25 + index)
    )
    coupling = odb.dbCCSeg.create(pin_cap, other_cap, True)
    for corner in range(block.getCornerCount()):
        coupling.setCapacitance(0.25 * (index + 1) * (corner + 1), corner)
    instances.append(instance)
    iterms.append(iterm)
    iterm_cap_ids.append(pin_cap.getId())
    iterm_couplings.append(coupling)
assert len(list(block.getITerms())) == 2
assert len(list(block.getInsts())) == 2

corners = range(block.getCornerCount())
selected_ids = {int(n) for n in TEST_NETS.split()}
reference_nets = [odb.dbNet.getNet(block, n) for n in sorted(selected_ids)]


def net_totals():
    return {
        (kind, corner, n.getId()): value
        for n in reference_nets
        for corner in corners
        for kind, value in [
            ("netCap", n.getTotalCapacitance(corner, True)),
            ("netRes", n.getTotalResistance(corner)),
        ]
    }


def cap_name(cap):
    if cap.isBTerm():
        return odb.dbBTerm.getBTerm(block, cap.getNode()).getName()
    if cap.isITerm():
        term = odb.dbITerm.getITerm(block, cap.getNode())
        return term.getInst().getName() + ":" + term.getMTerm().getName()
    assert cap.isInternal(), cap.getId()
    return cap.getNet().getName() + ":" + str(cap.getNode())


# Capture independent DB truth before export, not from possibly suppressed
# matching report rows. Include both native CC endpoint IDs and corner values.
reference_totals = net_totals()
reference_ccs = {}
reference_cc_objects = []
for coupling in block.getCCSegs():
    source = coupling.getSourceCapNode()
    target = coupling.getTargetCapNode()
    net_ids = (source.getNet().getId(), target.getNet().getId())
    if not selected_ids.intersection(net_ids):
        continue
    names = (cap_name(source), cap_name(target))
    assert names not in reference_ccs, ("duplicate reference coupling", names)
    reference_ccs[names] = {
        "ids": (source.getId(), target.getId()),
        "net_ids": net_ids,
        "net_names": (source.getNet().getName(), target.getNet().getName()),
        "values": tuple(coupling.getCapacitance(c) for c in corners),
    }
    reference_cc_objects.append(coupling)

reference = Path(helpers.make_result_file("diff_spef.spef")).resolve()
(reference.parent / "diff_spef-reference-db.json").write_text(
    json.dumps(
        {
            "net_totals": [[*key, value] for key, value in reference_totals.items()],
            "couplings": [
                {"names": names, **data} for names, data in reference_ccs.items()
            ],
        },
        indent=2,
    )
)
print(
    "Reference DB zero CC values:",
    json.dumps(
        [
            {"names": names, **data}
            for names, data in reference_ccs.items()
            if all(value == 0 for value in data["values"])
        ]
    ),
)


def positive_reference(raw, output):
    lines = raw.read_text().splitlines()
    assert "*C_UNIT 1 PF" in lines
    name_map = {
        fields[0]: fields[1]
        for line in lines
        if len(fields := line.split()) == 2
        and fields[0].startswith("*")
        and fields[0][1:].isdigit()
    }

    def resolve(word):
        name, separator, node = word.partition(":")
        return name_map.get(name, name) + separator + node

    kept = []
    diagnostics = Counter()
    section = None
    positive = 0
    for line in lines:
        if line.startswith("*"):
            section = line.split()[0]
        fields = line.split()
        if section == "*RES" and fields and not line.startswith("*"):
            assert len(fields) == 4, ("Invalid reference resistor", line)
        if section == "*CAP" and len(fields) == 4:
            names = tuple(resolve(word) for word in fields[1:3])
            data = reference_ccs[names]
            values = tuple(float(v) * 1000 for v in fields[3].split(":"))
            assert len(values) == len(corners)
            assert all(
                math.isclose(value, db, rel_tol=1e-5, abs_tol=1e-7)
                for value, db in zip(values, data["values"])
            ), (names, values, data)
            if any(value <= 0 for value in values):
                # Omit only an exactly mapped all-zero REF/DB record. Test
                # its existing diagnostic contract separately using raw SPEF.
                assert all(value == 0 for value in values), (names, values)
                assert all(value == 0 for value in data["values"]), data
                diagnostic = (
                    "Invalid",
                    "ref",
                    "ccCap",
                    "for",
                    "nodeIds",
                    *(str(i) for i in data["ids"]),
                    str(data["net_ids"][0]),
                    data["net_names"][0],
                    str(data["net_ids"][1]),
                    data["net_names"][1],
                )
                diagnostics[diagnostic] += len(corners)
                continue
            positive += 1
        kept.append(line)
    assert positive, "No positive reference coupling records"
    output.write_text("\n".join(kept) + "\n")
    return diagnostics


def expand_name_map(raw, output):
    lines = raw.read_text().splitlines(keepends=True)
    starts = [i for i, line in enumerate(lines) if line.strip() == "*NAME_MAP"]
    assert len(starts) == 1, "Expected one native fixture NAME_MAP"
    start = starts[0]
    end = start + 1
    names = {}
    while end < len(lines):
        fields = lines[end].split()
        if fields:
            if not re.fullmatch(r"\*[0-9]+", fields[0]):
                break
            assert len(fields) == 2 and fields[0] not in names, fields
            names[fields[0]] = fields[1]
        end += 1
    assert names and end < len(lines)
    expanded = lines[:start]
    replacements = 0
    for line in lines[end:]:
        parts = re.split(r"(\s+)", line)
        originals = {}
        for index, word in enumerate(parts):
            match = re.fullmatch(r"(\*[0-9]+)(:.*)?", word)
            if match:
                originals[index] = word
                parts[index] = names[match[1]] + (match[2] or "")
        rewritten = "".join(parts)
        # Invert only the replaced token positions. Every other byte, including
        # numeric values and pin/internal-node suffixes, must round-trip exactly.
        restored = re.split(r"(\s+)", rewritten)
        assert len(restored) == len(parts)
        for index, word in originals.items():
            restored[index] = word
        assert "".join(restored) == line
        assert not any(re.fullmatch(r"\*[0-9]+(?::.*)?", p) for p in parts)
        replacements += len(originals)
        expanded.append(rewritten)
    assert replacements, "No mapped fixture identifiers expanded"
    assert not any(line.strip() == "*NAME_MAP" for line in expanded)
    assert expanded[:start] == lines[:start], "SPEF header changed"
    output.write_text("".join(expanded))


raw_reference = reference.parent / "diff_spef-raw.spef"
write = rcx.SpefOptions()
write.file = str(raw_reference)
write.nets = TEST_NETS
extractor.write_spef(write)
zero_diagnostics = positive_reference(raw_reference, reference)
assert zero_diagnostics, "Zero-reference diagnostic fixture missing"

unmapped_raw = reference.parent / "diff_spef-no-name-map-raw.spef"
unmapped = reference.parent / "diff_spef-no-name-map.spef"
expand_name_map(raw_reference, unmapped_raw)
assert positive_reference(unmapped_raw, unmapped) == zero_diagnostics

selected_raw = reference.parent / "diff_spef-selected-raw.spef"
selected_reference = reference.parent / "diff_spef-selected.spef"
write.file = str(selected_raw)
write.nets = str(net.getId())
extractor.write_spef(write)
assert not positive_reference(selected_raw, selected_reference)
assert net_totals() == reference_totals
for coupling in reference_cc_objects:
    source = coupling.getSourceCapNode()
    target = coupling.getTargetCapNode()
    data = reference_ccs[(cap_name(source), cap_name(target))]
    assert (source.getId(), target.getId()) == data["ids"]
    assert tuple(coupling.getCapacitance(c) for c in corners) == data["values"]
assert internal.getNode() == 97

# Preserve pre-existing flags too, including a net absent from the reference.
unrelated = odb.dbNet.getNet(block, 4)
unrelated.setMark(True)
unrelated.setSpef(True)
first_bterm = next(iter(block.getBTerms()))
first_bterm.setExtId(0)
iterms[0].setExtId(0)
iterms[1].setExtId(internal.getId())
instances[0].setUserFlag1()
instances[1].clearUserFlag1()


def compare(
    label,
    api="python",
    filename=reference,
    error=None,
    net_names=None,
    m_map=False,
    diagnostics=None,
):
    directory = reference.parent / ("diff_spef-" + label)
    directory.mkdir(exist_ok=True)
    os.chdir(directory)
    before = snapshot(block)
    caught = None
    tcl_result = None
    try:
        if api == "tcl":
            assert net_names is None, "Tcl diff_spef has no net-selection option"
            # evalTclString returns the result but discards Tcl_Eval's status.
            tcl_result = design.evalTclString(
                "set diff_spef_status [catch {diff_spef -file {"
                + str(filename)
                + "} -r_conn -r_res -r_cap -r_cc_cap} diff_spef_result]; "
                + 'format "%d\\n%s" $diff_spef_status $diff_spef_result'
            )
        else:
            options = rcx.DiffOptions()
            options.file = str(filename)
            options.m_map = m_map
            options.low_guard = -0.15
            options.upper_guard = 0.15
            if net_names is not None:
                options.net = net_names
            options.r_conn = options.r_res = options.r_cap = options.r_cc_cap = True
            extractor.diff_spef(options)
    except RuntimeError as exception:
        caught = str(exception)
    after = snapshot(block)
    assert after == before, (
        label,
        "comparison changed DB parasitics",
        summary(before),
        summary(after),
    )
    if api == "tcl":
        assert tcl_result is not None, (label, caught)
        status, separator, message = tcl_result.partition("\n")
        assert separator and status in {"0", "1"}, (label, tcl_result)
        (directory / "tcl-result.json").write_text(
            json.dumps({"status": int(status), "result": message})
        )
        assert int(status) == (1 if error is not None else 0), (label, tcl_result)
        if status == "1":
            caught = message
    if error is not None:
        assert caught == error, (label, caught, error)
        return
    assert caught is None, (label, caught)
    log = directory / "diff_spef.log"
    assert log.is_file(), label
    if diagnostics is None:
        assert not log.read_text(), (label, "unexpected diff log")
    else:
        actual = Counter(tuple(line.split()) for line in log.read_text().splitlines())
        assert actual == diagnostics, (label, actual, diagnostics)
    return metrics(directory / "diff_spef.out")


for api in ["python", "tcl"]:
    matching = compare("matching-" + api, api)
    zeros = compare(
        "zero-reference-" + api, api, raw_reference, diagnostics=zero_diagnostics
    )
    for nets, couplings in [matching, zeros]:
        # Python uses ordered guards; Tcl keeps its native emission policy.
        # Equal-valued records may be suppressed, so presence is not required.
        assert all(abs(row[0]) <= 0.15 for row in [*nets.values(), *couplings.values()])

# Change known RC and both real ITerm couplings in each corner, not the SPEF.
# Nonzero percentages must now be reported by both APIs, with correct IDs.
for corner in corners:
    segment.setCapacitance(segment.getCapacitance(corner, 1.0) + 0.5, corner)
    segment.setResistance(segment.getResistance(corner) + 10.0, corner)
    for index, (rseg, coupling) in enumerate(zip(iterm_segments, iterm_couplings)):
        rseg.setCapacitance(
            rseg.getCapacitance(corner, 1.0) + 0.5 * (index + 1), corner
        )
        rseg.setResistance(rseg.getResistance(corner) + 5.0 * (index + 1), corner)
        coupling.setCapacitance(coupling.getCapacitance(corner) * (2 + index), corner)
changed_totals = net_totals()


def require_differences(result, only_net=False):
    nets, couplings = result
    if only_net:
        assert all(key[2] == net.getId() for key in nets)
    for corner in corners:
        for kind in ["netCap", "netRes"]:
            key = (kind, corner, net.getId())
            expected = (changed_totals[key] / reference_totals[key] - 1) * 100
            assert expected > 1, (key, expected)
            percent, db, ref = nets[key]
            assert abs(percent - expected) <= 0.15, (key, percent, expected)
            assert math.isclose(db, changed_totals[key], rel_tol=1e-5, abs_tol=0.0001)
            assert math.isclose(
                ref, reference_totals[key], rel_tol=1e-5, abs_tol=0.0001
            )
        for index, coupling in enumerate(iterm_couplings):
            source = coupling.getSourceCapNode()
            target = coupling.getTargetCapNode()
            assert source.getId() == iterm_cap_ids[index]
            data = reference_ccs[(cap_name(source), cap_name(target))]
            key = (corner, *data["ids"])
            percent, db, ref = couplings[key]
            expected = 100.0 * (1 + index)
            assert abs(percent - expected) <= 0.15, (key, percent, expected)
            assert math.isclose(db, coupling.getCapacitance(corner), rel_tol=1e-5)
            assert math.isclose(ref, data["values"][corner], rel_tol=1e-5)
    assert all(
        abs(row[0]) <= 0.15 for key, row in nets.items() if key[2] != net.getId()
    )


for api in ["python", "tcl"]:
    require_differences(compare("different-" + api, api))
    require_differences(compare("no-name-map-" + api, api, unmapped))
    require_differences(
        compare(
            "selected-net-" + api,
            api,
            filename=reference if api == "python" else selected_reference,
            net_names=net.getName() if api == "python" else None,
        ),
        only_net=True,
    )

compare(
    "missing-file", filename=reference.parent / "not-present.spef", error="RCX-0428"
)
invalid = reference.parent / "diff_spef-invalid.spef"
# A mapped net absent from the DB reaches the existing unmatched-input error
# after reading valid preceding nets; no transactional DB rollback is needed.
lines = reference.read_text().splitlines()
missing_net = odb.dbNet.getNet(block, 671)
for index, line in enumerate(lines):
    if line.split() == ["*671", missing_net.getName()]:
        lines[index] = "*671 missing_diff_net"
        break
else:
    raise AssertionError("Missing reference name-map entry")
invalid.write_text("\n".join(lines) + "\n")
compare("unmatched-net", filename=invalid, error="RCX-0052")


def missing_cross_net_reference(original, mapped):
    lines = original.read_text().splitlines()
    if mapped:
        map_id = 1 + max(
            int(fields[0][1:])
            for line in lines
            if len(fields := line.split()) == 2
            and fields[0].startswith("*")
            and fields[0][1:].isdigit()
        )
        lines.insert(lines.index("*NAME_MAP") + 1, f"*{map_id} missing_diff_cross_net")
        target = f"*{map_id}:97"
    else:
        target = "missing_diff_cross_net:97"
    in_caps = False
    for index, line in enumerate(lines):
        if line.startswith("*"):
            in_caps = line.startswith("*CAP")
        elif in_caps and len(fields := line.split()) == 4:
            fields[2] = target
            lines[index] = " ".join(fields)
            break
    else:
        raise AssertionError("No cross-net coupling in reference")
    path = reference.parent / ("diff_spef-missing-cross-net-" + str(mapped) + ".spef")
    path.write_text("\n".join(lines) + "\n")
    return path


for original, mapped in [(reference, True), (unmapped, False)]:
    missing_cross_net = missing_cross_net_reference(original, mapped)
    for api in ["python", "tcl"]:
        compare(
            "missing-cross-net-" + str(mapped) + "-" + api,
            api,
            filename=missing_cross_net,
            error="RCX-0052",
            m_map=mapped,
        )
for api in ["python", "tcl"]:
    require_differences(compare("after-errors-" + api, api))
assert internal.getNode() == 97
assert [iterm.getExtId() for iterm in iterms] == [0, internal.getId()]
assert [instance.getUserFlag1() for instance in instances] == [True, False]

# Default reading is still an import, not a comparison or calibration.
cap_count = len(list(block.getCapNodes()))
read = rcx.ReadSpefOpts()
read.file = str(reference)
read.force = True
extractor.read_spef(read)
net = odb.dbNet.getNet(block, 3)
assert len(list(block.getCapNodes())) < cap_count
# Import may intentionally regenerate derived corners; unlike pure comparison,
# it is checked against the exported base-corner values, not custom scaled RC.
assert (
    abs(net.getTotalCapacitance(0, True) - reference_totals[("netCap", 0, 3)]) < 0.0001
)
assert abs(net.getTotalResistance(0) - reference_totals[("netRes", 0, 3)]) < 0.0001
print("pass")
