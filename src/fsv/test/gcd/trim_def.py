"""Trims a routed DEF to the conductors near some nets, so that any window
inside the region reads the same shapes as in the full design. It keeps the
cells touching the region, the block pins in it (and the special nets'
pins), every net with routing or a pin there, and of each net (special nets
included) the routing statements that come within the region. Vias and the
die area are unchanged. The region is each target net's routing bounding
box grown by `reach` um."""

import re
import sys

def_path, lef_path, out_path, reach_um, *targets = sys.argv[1:]
reach = float(reach_um) * 1000
text = open(def_path).read()

# Cell sizes from the LEF, in DEF units.
sizes = {}
lef = open(lef_path).read()
for macro in lef.split("\nMACRO ")[1:]:
    name = macro.split()[0]
    m = re.search(r"\bSIZE\s+(\S+)\s+BY\s+(\S+)", macro)
    if m:
        sizes[name] = (float(m.group(1)) * 1000, float(m.group(2)) * 1000)


def section(name):
    start = text.index("\n" + name + " ")
    end = text.index("\nEND " + name, start)
    return start, end


def entries(body):
    return re.split(r"\n(?=\s+- )", body)[1:]


def points(block):
    out, last = [], None
    for x, y in re.findall(r"\(\s*(\S+)\s+(\S+)(?:\s+\S+)?\s*\)", block):
        try:
            x = last[0] if x == "*" else int(x)
            y = last[1] if y == "*" else int(y)
        except ValueError:
            continue
        last = (x, y)
        out.append(last)
    return out


ns, ne = section("NETS")
nets = entries(text[ns:ne])
regions = []
for block in nets:
    name = block.split()[1]
    if name in targets:
        pts = points(block[block.index("+ ROUTED") :])
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        regions.append(
            (min(xs) - reach, min(ys) - reach, max(xs) + reach, max(ys) + reach)
        )
assert len(regions) == len(targets), "a target net was not found"


def touches(x0, y0, x1, y1):
    return any(x0 <= r[2] and r[0] <= x1 and y0 <= r[3] and r[1] <= y1 for r in regions)


# A net's routing statements (paths and vias) that come within the region;
# None if none do. Each statement's "*" refers to its own previous point.
def trim_routing(block):
    if "+ ROUTED" not in block:
        return None
    start = block.index("+ ROUTED")
    head, routing = block[:start], block[start + len("+ ROUTED") :]
    end = routing.rfind(";")
    tail = routing[end:] if end >= 0 else ""
    routing = routing[:end] if end >= 0 else routing
    kept = []
    for stmt in re.split(r"\bNEW\b", routing):
        pts = points(stmt)
        if not pts:
            continue
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        if touches(min(xs), min(ys), max(xs), max(ys)):
            kept.append(stmt.strip())
    if not kept:
        return None
    return head + "+ ROUTED " + "\n      NEW ".join(kept) + " " + tail


# Cells touching the region.
cs, ce = section("COMPONENTS")
kept_cells = []
cell_names = set()
for block in entries(text[cs:ce]):
    words = block.split()
    name, master = words[1], words[2]
    m = re.search(r"\+ (?:PLACED|FIXED) \(\s*(-?\d+)\s+(-?\d+)\s*\)\s+(\S+)", block)
    x, y, orient = int(m.group(1)), int(m.group(2)), m.group(3)
    w, h = sizes[master]
    if orient in ("E", "W", "FE", "FW"):
        w, h = h, w
    if touches(x, y, x + w, y + h):
        kept_cells.append(block)
        cell_names.add(name)

# Block pins: special nets' pins whole, others if they lie in the region.
ps, pe = section("PINS")
kept_pins = []
pin_names = set()
pin_nets = set()
for block in entries(text[ps:pe]):
    name = block.split()[1]
    net = re.search(r"\+ NET (\S+)", block).group(1)
    special = "+ SPECIAL" in block
    m = re.search(r"\+ (?:PLACED|FIXED) \(\s*(-?\d+)\s+(-?\d+)\s*\)", block)
    inside = m and touches(
        int(m.group(1)), int(m.group(2)), int(m.group(1)), int(m.group(2))
    )
    if special or inside:
        kept_pins.append(block)
        pin_names.add(name)
        pin_nets.add(net)


def connects(block):
    """Whether a net has a pin on a kept cell or a kept block pin."""
    head = block.split("+")[0]
    for a, b in re.findall(r"\(\s*(\S+)\s+(\S+)\s*\)", head):
        if (a == "PIN" and b in pin_names) or a in cell_names:
            return True
    return False


# Nets with routing in the region, or with a pin in it (on a kept cell or a
# kept block pin): a pin whose net were dropped would read as unconnected,
# and so grounded. A net may keep no routing at all.
kept_nets = {}
for block in nets:
    name = block.split()[1]
    trimmed = block if name in targets else trim_routing(block)
    if trimmed is None and (connects(block) or name in pin_nets):
        trimmed = block.split("+ ROUTED")[0].rstrip() + " ;"
    if trimmed is not None:
        kept_nets[name] = trimmed


# Kept nets lose their connections to cells and pins that were dropped.
def trim_connections(block):
    head, sep, rest = block.partition("+")

    def keep(m):
        a, b = m.group(1), m.group(2)
        if a == "PIN":
            return m.group(0) if b in pin_names else ""
        return m.group(0) if a in cell_names else ""

    head = re.sub(r"\(\s*(\S+)\s+(\S+)\s*\)", keep, head)
    return re.sub(r"[ \t]+", " ", head) + sep + rest


def rebuild(start, end, name, blocks):
    body = "\n".join(blocks)
    return f"\n{name} {len(blocks)} ;\n" + body.lstrip("\n") + "\n"


ss, se = section("SPECIALNETS")
specials = [
    trim_routing(b) or b.split("+ ROUTED")[0].rstrip() + " ;"
    for b in entries(text[ss:se])
]
out = text[:cs] + rebuild(cs, ce, "COMPONENTS", kept_cells) + text[ce:ps]
out += rebuild(ps, pe, "PINS", kept_pins) + text[pe:ss]
out += rebuild(ss, se, "SPECIALNETS", specials) + text[se:ns]
out += (
    rebuild(ns, ne, "NETS", [trim_connections(b) for b in kept_nets.values()])
    + text[ne:]
)
# Rows and tracks are not needed to read the conductors.
out = re.sub(r"^(ROW|TRACKS|GCELLGRID) .*\n", "", out, flags=re.M)
open(out_path, "w").write(out)
print(
    f"cells {len(kept_cells)} pins {len(kept_pins)} nets {len(kept_nets)} -> {len(out)} bytes"
)
