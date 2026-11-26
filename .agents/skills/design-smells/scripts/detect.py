#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Report design smells from a facts file written by extract.py.

Smells (Lanza & Marinescu, "Object-Oriented Metrics in Practice"):
  feature-envy    method uses another class's data more than its own
  god-class       large, non-cohesive class that leans on others' data
  data-class      class that is mostly data plus accessors
  message-chain   a->b()->c()->d() navigation through several classes
  intimacy        two classes that each use many of the other's members

Every threshold is a flag, so results can be tuned without re-extracting.
See references/smells.md for metric definitions and triage guidance.
"""

import argparse
import itertools
import json
import re
import sys
from collections import defaultdict

SMELLS = ["feature-envy", "god-class", "data-class", "message-chain", "intimacy"]

# Members of these classes never count as foreign data: value types,
# containers, and infrastructure every class legitimately uses.
DEFAULT_IGNORE_CLASS = (
    r"^(std|boost|fmt|absl|Eigen|spdlog|gtl|__gnu_cxx|google|py|nlohmann)::"
    r"|^Q[A-Z]\w*$"
    r"|^utl::(Logger|Timer|DebugScopedTimer)$"
    r"|^odb::(Rect|Point|Point3D|Line|Polygon|Oct|Cuboid|Orientation\w*)$"
    r"|^odb::dbSet|^odb::dbSetIterator"
    r"|^\w+::(Point|Rect|Box|Point3D|Polygon|Interval|Segment)$"
)
# Methods whose job is to read another object's data: serializers,
# property descriptors, parsers.  Skipped for feature envy only.
DEFAULT_FE_EXCLUDE = (
    r"(Writer|Reader|Parser|Descriptor|Serializer)::|::(write|read|dump|print)"
    r"[A-Z_]\w*$|::(lefout|defout|definReader|lefin)::"
)


def short(name):
    return re.sub(r"^\w+::", "", name)


def loc(item):
    return f"{item['file']}:{item['line']}"


class Facts:
    def __init__(self, data, ignore_re, path_re):
        self.classes = data["classes"]
        self.methods = data["methods"]
        self.ignore = ignore_re
        self.path_re = path_re
        self.by_class = defaultdict(list)
        for m in self.methods.values():
            self.by_class[m["class_usr"]].append(m)
        self.class_name_to_usr = {c["name"]: usr for usr, c in self.classes.items()}

    def reported(self, item):
        return self.path_re is None or self.path_re.search(item["file"])

    def foreign(self, m):
        """{class: {member: (kind, count)}} without ignored classes."""
        return {c: v for c, v in m["foreign"].items() if not self.ignore.search(c)}


def feature_envy(facts, a):
    fe_exclude = re.compile(a.fe_exclude) if a.fe_exclude else None
    out = []
    for m in facts.methods.values():
        if m["kind"] != "method" or not facts.reported(m):
            continue
        if fe_exclude and fe_exclude.search(m["name"]):
            continue
        foreign = facts.foreign(m)
        atfd = sum(len(v) for v in foreign.values())
        if atfd <= a.atfd:
            continue
        own = len(m["own_fields"]) + len(m["own_methods"])
        laa = own / (own + atfd)
        fdp = len(foreign)
        if laa >= a.laa or fdp > a.fdp:
            continue
        envied = sorted(
            foreign.items(),
            key=lambda kv: (-len(kv[1]), -sum(n for _, n in kv[1].values())),
        )
        top_cls, top = envied[0]
        fields_only = all(kind == "field" for kind, _ in top.values())
        if fields_only and not a.include_data:
            continue
        out.append(
            {
                "smell": "feature-envy",
                "method": m["name"],
                "file": m["file"],
                "line": m["line"],
                "lines": m["lines"],
                "atfd": atfd,
                "laa": round(laa, 2),
                "fdp": fdp,
                "envied": top_cls,
                "envied_members": sorted(top),
                "fields_only": fields_only,
                "score": atfd * (1 - laa),
            }
        )
    out.sort(key=lambda r: -r["score"])
    return out


def class_metrics(facts, usr, c):
    methods = facts.by_class.get(usr, [])
    wmc = sum(m["complexity"] for m in methods)
    foreign = set()
    for m in methods:
        for cls, members in facts.foreign(m).items():
            foreign.update((cls, name) for name in members)
    # TCC over public, non-static, non-special methods with a body here.
    decl = {d["usr"]: d for d in c["methods"]}
    visible = []
    for usr_m, m in facts.methods.items():
        d = decl.get(usr_m)
        if (
            m["class_usr"] == usr
            and m["kind"] == "method"
            and not m["static"]
            and (d is None or d["access"] == "public")
        ):
            visible.append(set(m["own_fields"]))
    pairs = len(visible) * (len(visible) - 1) // 2
    connected = sum(1 for x, y in itertools.combinations(visible, 2) if x & y)
    tcc = connected / pairs if pairs else None
    return wmc, len(foreign), tcc, len(methods)


def field_clusters(facts, usr):
    """Groups of methods linked by the own fields they share.

    Fields used by more than a quarter of the methods (hubs such as the
    logger or the database) would join everything, so they are left out of
    the linking.  Each group of fields and methods is a candidate class to
    extract; one giant group means there is no clean split.
    """
    methods = [m for m in facts.by_class.get(usr, []) if m["kind"] == "method"]
    if not methods:
        return [], []
    uses = defaultdict(int)
    for m in methods:
        for f in m["own_fields"]:
            uses[f] += 1
    hubs = sorted(f for f, n in uses.items() if n > len(methods) / 4)
    parent = {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for m in methods:
        fields = [f for f in m["own_fields"] if f not in hubs]
        for f in fields:
            find(f)
        for f in fields[1:]:
            parent[find(f)] = find(fields[0])
    groups = defaultdict(lambda: {"fields": set(), "methods": []})
    for f in parent:
        groups[find(f)]["fields"].add(f)
    for m in methods:
        fields = [f for f in m["own_fields"] if f not in hubs]
        if fields:
            groups[find(fields[0])]["methods"].append(m["name"].rsplit("::", 1)[-1])
    clusters = [
        {"fields": sorted(g["fields"]), "methods": sorted(set(g["methods"]))}
        for g in groups.values()
        if len(set(g["methods"])) >= 2
    ]
    clusters.sort(key=lambda g: -len(g["methods"]))
    return clusters, hubs


def god_class(facts, a):
    out = []
    for usr, c in facts.classes.items():
        if not facts.reported(c):
            continue
        wmc, atfd, tcc, n = class_metrics(facts, usr, c)
        if wmc < a.wmc or atfd <= a.class_atfd or tcc is None or tcc >= a.tcc:
            continue
        out.append(
            {
                "smell": "god-class",
                "class": c["name"],
                "file": c["file"],
                "line": c["line"],
                "wmc": wmc,
                "tcc": round(tcc, 2),
                "atfd": atfd,
                "methods": n,
                "fields": len(c["fields"]),
                "score": wmc * (1 - tcc),
            }
        )
        clusters, hubs = field_clusters(facts, usr)
        out[-1].update(
            {
                "clusters": clusters,
                "hub_fields": hubs,
            }
        )
    out.sort(key=lambda r: -r["score"])
    return out


def data_class(facts, a, envy):
    users = defaultdict(set)
    for r in envy:
        users[r["envied"]].add(r["method"])
    accessor_usrs = {u for u, m in facts.methods.items() if m["accessor"]}
    out = []
    for usr, c in facts.classes.items():
        if not facts.reported(c):
            continue
        public_methods = [
            d
            for d in c["methods"]
            if d["access"] == "public"
            and d["kind"] == "method"
            and not d["static"]
            and not d["defaulted"]
        ]
        if not public_methods and not a.include_aggregates:
            continue  # plain aggregate: a deliberate bundle of values
        nopa = sum(1 for f in c["fields"] if f["access"] == "public")
        noam = sum(1 for d in public_methods if d["usr"] in accessor_usrs)
        functional = len(public_methods) - noam
        public_members = len(public_methods) + nopa
        if public_members == 0:
            continue
        woc = functional / public_members
        wmc = sum(m["complexity"] for m in facts.by_class.get(usr, []))
        exposed = nopa + noam
        if woc >= a.woc:
            continue
        if not ((exposed > 2 and wmc < 31) or (exposed > 4 and wmc < 47)):
            continue
        out.append(
            {
                "smell": "data-class",
                "class": c["name"],
                "file": c["file"],
                "line": c["line"],
                "woc": round(woc, 2),
                "public_fields": nopa,
                "accessors": noam,
                "wmc": wmc,
                "envied_by": sorted(users.get(c["name"], []))[:10],
                "score": exposed * (1 - woc),
            }
        )
    out.sort(key=lambda r: (-len(r["envied_by"]), -r["score"]))
    return out


def chain_links(facts, ch):
    links = ch["chain"]
    # Drop the own-class prefix (this, own fields) and value types.
    while links and links[0][2]:
        links = links[1:]
    # Implicit conversions (enum wrappers to their Value) are not navigation.
    return [
        x
        for x in links
        if not facts.ignore.search(x[0]) and not x[1].startswith("operator")
    ]


def repeated_subchains(facts, a):
    """Runs of navigation (a->getB()->getC()) repeated at many sites,
    wherever they start.  Short chains slip under --chain, but repeated
    dozens of times they are still the case for a shortcut accessor."""
    sites = defaultdict(set)
    for m in facts.methods.values():
        if not facts.reported(m):
            continue
        for ch in m["chains"]:
            links = [(c, n) for c, n, _ in chain_links(facts, ch)]
            for n in range(2, min(len(links), 4) + 1):
                for i in range(len(links) - n + 1):
                    gram = tuple(links[i : i + n])
                    if len({c for c, _ in gram}) >= 2:
                        sites[gram].add((m["file"], ch["line"], m["name"]))
    frequent = {g: v for g, v in sites.items() if len(v) >= a.subchain_count}
    out = []
    for gram, where in frequent.items():
        # A run that mostly occurs inside a longer frequent run is reported
        # as that longer run.
        if any(
            len(other) > len(gram)
            and len(frequent[other]) >= 0.8 * len(where)
            and any(other[i : i + len(gram)] == gram for i in range(len(other)))
            for other in frequent
        ):
            continue
        first = min(where)
        out.append(
            {
                "smell": "repeated-subchain",
                "chain": " -> ".join(f"{short(c)}::{n}" for c, n in gram),
                "length": len(gram),
                "count": len(where),
                "methods": len({w[2] for w in where}),
                "file": first[0],
                "line": first[1],
                "sites": [f"{f}:{ln} {short(n)}" for f, ln, n in sorted(where)],
            }
        )
    out.sort(key=lambda r: (-r["count"] * r["length"], r["chain"]))
    return out


def message_chains(facts, a):
    """Identical chains grouped, most repeated first: a repeated chain is
    the case for a shortcut method on the class where it starts."""
    groups = {}
    for m in facts.methods.values():
        if not facts.reported(m):
            continue
        for ch in m["chains"]:
            links = chain_links(facts, ch)
            classes = {x[0] for x in links}
            if len(links) < a.chain or len(classes) < a.chain_classes:
                continue
            text = " -> ".join(f"{short(c)}::{n}" for c, n, _ in links)
            g = groups.setdefault(
                text,
                {
                    "smell": "message-chain",
                    "chain": text,
                    "length": len(links),
                    "file": m["file"],
                    "line": ch["line"],
                    "sites": [],
                },
            )
            g["sites"].append(f"{m['file']}:{ch['line']} {short(m['name'])}")
    out = list(groups.values())
    for g in out:
        g["count"] = len(g["sites"])
    out.sort(key=lambda r: (-r["count"] * r["length"], r["chain"]))
    return out


def intimacy(facts, a):
    uses = defaultdict(lambda: defaultdict(set))  # A -> B -> members
    files = {}
    for m in facts.methods.values():
        files.setdefault(m["class"], (m["file"], m["line"]))
        for cls, members in facts.foreign(m).items():
            uses[m["class"]][cls].update(members)
    out = []
    for x, targets in uses.items():
        for y, xy in targets.items():
            if x >= y:
                continue
            yx = uses.get(y, {}).get(x, set())
            if len(xy) < a.intimacy or len(yx) < a.intimacy:
                continue
            # Nested classes are one unit by design.
            if y.startswith(x + "::") or x.startswith(y + "::"):
                continue
            file, line = files[x]
            item = {"file": file, "line": line}
            if not facts.reported(item):
                continue
            out.append(
                {
                    "smell": "intimacy",
                    "classes": [x, y],
                    "file": file,
                    "line": line,
                    "a_uses_b": sorted(xy),
                    "b_uses_a": sorted(yx),
                    "score": min(len(xy), len(yx)),
                }
            )
    out.sort(key=lambda r: -r["score"])
    return out


def print_table(title, rows, cols, top):
    print(f"\n## {title} ({len(rows)})")
    if not rows:
        return
    for r in rows[:top]:
        print("  " + "  ".join(fmt(r) for fmt in cols))
    if len(rows) > top:
        print(f"  ... {len(rows) - top} more (use --top or --json)")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("facts", help="JSON written by extract.py")
    ap.add_argument(
        "--smell",
        action="append",
        choices=SMELLS,
        help="smell to report (repeatable; default all)",
    )
    ap.add_argument("--path", help="only report items whose file matches regex")
    ap.add_argument("--top", type=int, default=25, help="rows per smell")
    ap.add_argument("--json", help="write all findings here")
    ap.add_argument("--ignore-class", default=DEFAULT_IGNORE_CLASS)
    g = ap.add_argument_group("feature envy")
    g.add_argument("--atfd", type=int, default=5, help="flag ATFD > N (5)")
    g.add_argument("--laa", type=float, default=1 / 3, help="flag LAA < X (1/3)")
    g.add_argument("--fdp", type=int, default=3, help="flag FDP <= N (3)")
    g.add_argument("--fe-exclude", default=DEFAULT_FE_EXCLUDE)
    g.add_argument(
        "--include-data",
        action="store_true",
        help="keep methods that only read fields of the envied class",
    )
    g = ap.add_argument_group("god class")
    g.add_argument("--wmc", type=int, default=47, help="flag WMC >= N (47)")
    g.add_argument("--tcc", type=float, default=1 / 3, help="flag TCC < X (1/3)")
    g.add_argument("--class-atfd", type=int, default=5, help="flag ATFD > N (5)")
    g.add_argument(
        "--clusters",
        action="store_true",
        help="list each class's field/method groups (candidate classes)",
    )
    g = ap.add_argument_group("data class")
    g.add_argument("--woc", type=float, default=1 / 3, help="flag WOC < X (1/3)")
    g.add_argument(
        "--include-aggregates",
        action="store_true",
        help="also flag structs with no public methods",
    )
    g = ap.add_argument_group("message chain")
    g.add_argument("--chain", type=int, default=4, help="links >= N (4)")
    g.add_argument(
        "--chain-classes", type=int, default=3, help="distinct classes >= N (3)"
    )
    g.add_argument(
        "--subchain-count",
        type=int,
        default=5,
        help="report sub-chains of 2+ links repeated at >= N sites (5)",
    )
    g = ap.add_argument_group("intimacy")
    g.add_argument(
        "--intimacy", type=int, default=4, help="members used each way >= N (4)"
    )
    a = ap.parse_args()

    with open(a.facts) as f:
        data = json.load(f)
    facts = Facts(
        data,
        re.compile(a.ignore_class),
        re.compile(a.path) if a.path else None,
    )
    smells = a.smell or SMELLS
    print(
        f"# Design smells: {len(facts.classes)} classes, "
        f"{len(facts.methods)} methods from {data['files_parsed']} files"
    )
    if data["parse_errors"]:
        print(
            f"warning: {len(data['parse_errors'])} files had parse errors; "
            "their results may be incomplete: "
            + ", ".join(sorted(data["parse_errors"])[:10])
        )

    findings = {}
    envy = feature_envy(facts, a)
    if "feature-envy" in smells:
        findings["feature-envy"] = envy
        print_table(
            "Feature Envy  [ATFD LAA FDP  location  method -> envied class]",
            envy,
            [
                lambda r: f"{r['atfd']:>3} {r['laa']:.2f} {r['fdp']}",
                loc,
                lambda r: f"{r['method']} -> {r['envied']}"
                + (" (fields only)" if r["fields_only"] else ""),
            ],
            a.top,
        )
    if "god-class" in smells:
        findings["god-class"] = god_class(facts, a)
        print_table(
            "God Class  [WMC TCC ATFD methods  location  class  (group sizes)]",
            findings["god-class"],
            [
                lambda r: f"{r['wmc']:>4} {r['tcc']:.2f} {r['atfd']:>3} "
                f"{r['methods']:>3}",
                loc,
                lambda r: f"{r['class']}  ("
                + "/".join(str(len(g["methods"])) for g in r["clusters"][:6])
                + ")",
            ],
            a.top,
        )
        if a.clusters:
            for r in findings["god-class"][: a.top]:
                print(f"\n  {r['class']}: hub fields {r['hub_fields']}")
                for g in r["clusters"][:8]:
                    print(
                        f"    {len(g['methods'])} methods  fields "
                        f"{', '.join(g['fields'][:8])}"
                        + (" ..." if len(g["fields"]) > 8 else "")
                    )
                    print(
                        f"      {', '.join(g['methods'][:10])}"
                        + (" ..." if len(g["methods"]) > 10 else "")
                    )
    if "data-class" in smells:
        findings["data-class"] = data_class(facts, a, envy)
        print_table(
            "Data Class  [WOC pub-fields accessors  location  class <- envied by]",
            findings["data-class"],
            [
                lambda r: f"{r['woc']:.2f} {r['public_fields']:>2} "
                f"{r['accessors']:>2}",
                loc,
                lambda r: r["class"]
                + (
                    f" <- {', '.join(short(x) for x in r['envied_by'][:3])}"
                    if r["envied_by"]
                    else ""
                ),
            ],
            a.top,
        )
    if "message-chain" in smells:
        findings["message-chain"] = message_chains(facts, a)
        print_table(
            "Message Chains  [sites links  first location  chain]",
            findings["message-chain"],
            [
                lambda r: f"{r['count']:>3} {r['length']:>2}",
                loc,
                lambda r: r["chain"],
            ],
            a.top,
        )
        findings["repeated-subchain"] = repeated_subchains(facts, a)
        print_table(
            "Repeated sub-chains  [sites methods  first location  chain]",
            findings["repeated-subchain"],
            [
                lambda r: f"{r['count']:>3} {r['methods']:>3}",
                loc,
                lambda r: r["chain"],
            ],
            a.top,
        )
    if "intimacy" in smells:
        findings["intimacy"] = intimacy(facts, a)
        print_table(
            "Inappropriate Intimacy  [A->B B->A  location  A <-> B]",
            findings["intimacy"],
            [
                lambda r: f"{len(r['a_uses_b']):>3} {len(r['b_uses_a']):>3}",
                loc,
                lambda r: f"{r['classes'][0]} <-> {r['classes'][1]}",
            ],
            a.top,
        )
    if a.json:
        with open(a.json, "w") as f:
            json.dump(findings, f, indent=1)
        print(f"\nall findings written to {a.json}", file=sys.stderr)


if __name__ == "__main__":
    main()
