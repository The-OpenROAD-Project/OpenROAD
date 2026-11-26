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
  shotgun-surgery method called from many methods in many other classes
  middle-man      class whose methods mostly forward to another class
Smells (Fowler, "Refactoring"):
  long-method     method or function too long or too complex
  long-params     too many parameters
  data-clump      the same group of parameters/fields recurring together

Every threshold is a flag, so results can be tuned without re-extracting.
See references/smells.md for metric definitions and triage guidance.
"""

import argparse
import itertools
import json
import re
import sys
from collections import defaultdict

SMELLS = [
    "feature-envy",
    "god-class",
    "data-class",
    "message-chain",
    "intimacy",
    "shotgun-surgery",
    "middle-man",
    "long-method",
    "long-params",
    "data-clump",
]

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
        if "functions" not in data:
            sys.exit("facts file predates version 2: rerun run.sh extract")
        self.functions = data["functions"]

    def reported(self, item):
        return self.path_re is None or self.path_re.search(item["file"])

    def trivial(self, m):
        """Getter-sized: no branches and at most five lines.  Such methods
        rarely change, so many callers are not a change risk."""
        return m["accessor"] or (m["complexity"] == 1 and m["lines"] <= 5)

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


def callables(facts):
    """Methods and free functions, with an owner name for each."""
    for m in facts.methods.values():
        yield m, m["class"]
    for f in facts.functions.values():
        yield f, f["file"]


def long_method(facts, a):
    out = []
    for m, owner in callables(facts):
        if not facts.reported(m):
            continue
        if m["lines"] <= a.max_lines and m["complexity"] <= a.max_cyclo:
            continue
        out.append(
            {
                "smell": "long-method",
                "method": m["name"],
                "file": m["file"],
                "line": m["line"],
                "lines": m["lines"],
                "cyclo": m["complexity"],
                "score": max(m["lines"] / a.max_lines, m["complexity"] / a.max_cyclo),
            }
        )
    out.sort(key=lambda r: -r["score"])
    return out


def long_params(facts, a):
    out = []
    for m, owner in callables(facts):
        # An override's signature is fixed by its base class.
        if not facts.reported(m) or m.get("override"):
            continue
        n = len(m["params"])
        if n <= a.max_params:
            continue
        out.append(
            {
                "smell": "long-params",
                "method": m["name"],
                "file": m["file"],
                "line": m["line"],
                "params": n,
                "ctor": m.get("kind") == "ctor",
                "signature": ", ".join(
                    f"{p['type']} {p['name']}".strip() for p in m["params"]
                ),
            }
        )
    out.sort(key=lambda r: -r["params"])
    return out


def data_clumps(facts, a):
    """Groups of 3+ parameters or fields that recur together.

    Each occurrence is a method signature (overloads and overrides of one
    method count once) or a class's field list.  Items match on type and
    name, or on name alone with --clump-by name.
    """

    def items(entries):
        keys = []
        for e in entries:
            if not e["name"]:
                continue
            keys.append(
                e["name"] if a.clump_by == "name" else f"{e['type']} {e['name']}"
            )
        return sorted(set(keys))

    occurrences = {}  # (kind, owner, name) -> (item set, file, line)
    for m, owner in callables(facts):
        if m.get("override") or len(m["params"]) < a.clump_size:
            continue
        key = ("params", owner, m["name"])
        occurrences.setdefault(key, (set(), m["file"], m["line"]))[0].update(
            items(m["params"])
        )
    for c in facts.classes.values():
        if len(c["fields"]) >= a.clump_size:
            occurrences[("fields", c["name"], c["name"])] = (
                set(items(c["fields"])),
                c["file"],
                c["line"],
            )

    where = defaultdict(set)
    for key, (its, _, _) in occurrences.items():
        its = sorted(its)
        if len(its) > a.clump_max_items:
            continue  # too many combinations; long-params reports these
        for k in range(a.clump_size, len(its) + 1):
            for combo in itertools.combinations(its, k):
                where[combo].add(key)
    frequent = {c: w for c, w in where.items() if len(w) >= a.clump_count}

    # Drop a group that mostly occurs inside a larger frequent group.
    best_super = defaultdict(int)
    for combo in sorted(frequent, key=len, reverse=True):
        top = max(len(frequent[combo]), best_super[combo])
        for sub in itertools.combinations(combo, len(combo) - 1):
            if sub in frequent:
                best_super[sub] = max(best_super[sub], top)
    out = []
    for combo, keys in frequent.items():
        if best_super[combo] >= 0.8 * len(keys):
            continue
        sites = sorted((occurrences[k][1], occurrences[k][2], k[0], k[2]) for k in keys)
        item = {"file": sites[0][0], "line": sites[0][1]}
        if not any(facts.reported({"file": f}) for f, _, _, _ in sites):
            continue
        out.append(
            {
                "smell": "data-clump",
                "items": list(combo),
                "count": len(keys),
                "in_fields": sum(1 for k in keys if k[0] == "fields"),
                "file": item["file"],
                "line": item["line"],
                "sites": [f"{f}:{ln} {kind} {short(n)}" for f, ln, kind, n in sites],
                "score": len(keys) * len(combo),
            }
        )
    out.sort(key=lambda r: -r["score"])
    return out


def middle_man(facts, a):
    """Classes where most public methods just forward one call."""
    out = []
    for usr, c in facts.classes.items():
        if not facts.reported(c):
            continue
        public = {
            d["usr"]
            for d in c["methods"]
            if d["access"] == "public" and d["kind"] == "method" and not d["static"]
        }
        methods = [
            m for u, m in facts.methods.items() if m["class_usr"] == usr and u in public
        ]
        if len(methods) < a.mm_min:
            continue
        targets = defaultdict(list)
        for m in methods:
            foreign = facts.foreign(m)
            called = [
                (cls, name)
                for cls, members in foreign.items()
                for name, (kind, _) in members.items()
                if kind == "method"
            ]
            fields = [
                1
                for members in foreign.values()
                for kind, _ in members.values()
                if kind == "field"
            ]
            if (
                m["stmts"] == 1
                and m["complexity"] == 1
                and not m["own_methods"]
                and len(called) == 1
                and not fields
            ):
                targets[called[0][0]].append(m["name"].rsplit("::", 1)[-1])
        delegating = sum(len(v) for v in targets.values())
        if delegating < a.mm_min or delegating / len(methods) < a.mm_ratio:
            continue
        target, names = max(targets.items(), key=lambda kv: len(kv[1]))
        # pimpl: forwarding to a nested Impl is the point of the idiom.
        if target.startswith(c["name"] + "::") and not a.include_pimpl:
            continue
        out.append(
            {
                "smell": "middle-man",
                "class": c["name"],
                "file": c["file"],
                "line": c["line"],
                "delegating": delegating,
                "methods": len(methods),
                "ratio": round(delegating / len(methods), 2),
                "target": target,
                "forwarders": sorted(names),
                "score": delegating * delegating / len(methods),
            }
        )
    out.sort(key=lambda r: -r["score"])
    return out


def shotgun_surgery(facts, a):
    """Methods whose callers are spread over many methods and classes, so
    changing the method means editing all of them."""
    callers = defaultdict(set)
    for m, owner in callables(facts):
        for callee in m["calls"]:
            callers[callee].add((m["name"], owner))
    out = []
    for usr, m in facts.methods.items():
        if m["kind"] != "method" or facts.trivial(m) or not facts.reported(m):
            continue
        cls = facts.classes.get(m["class_usr"])
        # A module's public API (include/) is meant to be called widely.
        if not a.include_api and cls and "/include/" in cls["file"]:
            continue
        outside = {(n, o) for n, o in callers.get(usr, ()) if o != m["class"]}
        cm = len(outside)
        cc = len({o for _, o in outside})
        if cm <= a.cm or cc <= a.cc:
            continue
        out.append(
            {
                "smell": "shotgun-surgery",
                "method": m["name"],
                "file": m["file"],
                "line": m["line"],
                "cm": cm,
                "cc": cc,
                "calling_classes": sorted({short(o) for _, o in outside})[:12],
                "score": cm * cc,
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
    g = ap.add_argument_group("shotgun surgery")
    g.add_argument("--cm", type=int, default=7, help="calling methods > N (7)")
    g.add_argument("--cc", type=int, default=5, help="calling classes > N (5)")
    g.add_argument(
        "--include-api",
        action="store_true",
        help="also check classes declared under include/ (module APIs)",
    )
    g = ap.add_argument_group("middle man")
    g.add_argument(
        "--mm-ratio", type=float, default=0.5, help="forwarding share >= X (0.5)"
    )
    g.add_argument("--mm-min", type=int, default=3, help="forwarders >= N (3)")
    g.add_argument(
        "--include-pimpl",
        action="store_true",
        help="keep classes that forward to their own nested Impl",
    )
    g = ap.add_argument_group("long method / long parameter list")
    g.add_argument("--max-lines", type=int, default=100, help="lines > N (100)")
    g.add_argument("--max-cyclo", type=int, default=25, help="complexity > N (25)")
    g.add_argument("--max-params", type=int, default=6, help="params > N (6)")
    g = ap.add_argument_group("data clump")
    g.add_argument("--clump-size", type=int, default=3, help="group size >= N (3)")
    g.add_argument("--clump-count", type=int, default=4, help="occurrences >= N (4)")
    g.add_argument(
        "--clump-by",
        choices=["type-name", "name"],
        default="type-name",
        help="match items on type and name (default) or name only",
    )
    g.add_argument(
        "--clump-max-items",
        type=int,
        default=12,
        help="skip signatures/field lists longer than N (12)",
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
    if "shotgun-surgery" in smells:
        findings["shotgun-surgery"] = shotgun_surgery(facts, a)
        print_table(
            "Shotgun Surgery  [CM CC  location  method <- calling classes]",
            findings["shotgun-surgery"],
            [
                lambda r: f"{r['cm']:>4} {r['cc']:>3}",
                loc,
                lambda r: f"{r['method']} <- "
                + ", ".join(r["calling_classes"][:4])
                + (" ..." if r["cc"] > 4 else ""),
            ],
            a.top,
        )
    if "middle-man" in smells:
        findings["middle-man"] = middle_man(facts, a)
        print_table(
            "Middle Man  [forwarders/public ratio  location  class -> target]",
            findings["middle-man"],
            [
                lambda r: f"{r['delegating']:>3}/{r['methods']:<3} {r['ratio']:.2f}",
                loc,
                lambda r: f"{r['class']} -> {r['target']}",
            ],
            a.top,
        )
    if "long-method" in smells:
        findings["long-method"] = long_method(facts, a)
        print_table(
            "Long Method  [lines cyclo  location  method]",
            findings["long-method"],
            [
                lambda r: f"{r['lines']:>5} {r['cyclo']:>4}",
                loc,
                lambda r: r["method"],
            ],
            a.top,
        )
    if "long-params" in smells:
        findings["long-params"] = long_params(facts, a)
        print_table(
            "Long Parameter List  [params  location  method]",
            findings["long-params"],
            [
                lambda r: f"{r['params']:>3}",
                loc,
                lambda r: r["method"] + (" (ctor)" if r["ctor"] else ""),
            ],
            a.top,
        )
    if "data-clump" in smells:
        findings["data-clump"] = data_clumps(facts, a)
        print_table(
            "Data Clumps  [sites (in fields)  first location  items]",
            findings["data-clump"],
            [
                lambda r: f"{r['count']:>3} ({r['in_fields']})",
                loc,
                lambda r: ", ".join(r["items"]),
            ],
            a.top,
        )
    if a.json:
        with open(a.json, "w") as f:
            json.dump(findings, f, indent=1)
        print(f"\nall findings written to {a.json}", file=sys.stderr)


if __name__ == "__main__":
    main()
