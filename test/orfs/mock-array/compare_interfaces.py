"""Compare MockArray and Element module interfaces between SV and Chisel.

The Chisel-generated Verilog flattens unpacked arrays into individual ports
(e.g. io_ins_down_0, io_ins_down_1, ...) while the SystemVerilog version uses
unpacked array syntax (e.g. io_ins_down [4]).  This script normalizes both
representations to {base_name: (direction, total_bits)} and compares them.
"""

import re
import sys


def parse_ports(text, module_name):
    """Parse ports from a (System)Verilog module declaration.

    Handles parameterized modules with #(...) blocks and continuation lines.
    Returns {port_name: (direction, width_expr, unpacked_expr)} with raw
    expressions (not yet evaluated).
    """
    # Find module declaration - handle optional #(...) parameter block
    # Use non-greedy matching to find the port list parentheses
    pattern = (
        r"module\s+"
        + re.escape(module_name)
        + r"\s*(?:#\s*\([^)]*\))?\s*\(([^)]*)\)\s*;"
    )
    m = re.search(pattern, text, re.DOTALL)
    if not m:
        return None

    body = m.group(1)
    ports = {}
    current_dir = None
    current_width_expr = "1"

    for line in body.split("\n"):
        line = line.strip().rstrip(",")
        if not line:
            continue

        # Full port declaration
        full = re.match(
            r"(input|output|inout)\s+"
            r"(?:logic\s+|wire\s+|reg\s+)?"
            r"(?:\[([^\]]+):([^\]]+)\]\s*)?"
            r"(\w+)"
            r"(?:\s*\[([^\]]+)\])?",
            line,
        )
        if full:
            current_dir = full.group(1)
            high_expr = full.group(2)
            low_expr = full.group(3)
            if high_expr is not None:
                current_width_expr = f"({high_expr})-({low_expr})+1"
            else:
                current_width_expr = "1"
            name = full.group(4)
            unpacked_expr = full.group(5) if full.group(5) else None
            ports[name] = (current_dir, current_width_expr, unpacked_expr)
            continue

        # Continuation line (inherits direction/width from previous)
        cont = re.match(r"(\w+)(?:\s*\[([^\]]+)\])?$", line)
        if cont and current_dir:
            name = cont.group(1)
            unpacked_expr = cont.group(2) if cont.group(2) else None
            ports[name] = (current_dir, current_width_expr, unpacked_expr)

    return ports


def eval_expr(expr, params):
    """Evaluate expression with parameter substitution."""
    if expr is None:
        return 1
    expr = expr.strip()
    # Strip Verilog macro backticks
    expr = expr.replace("`", "")
    for name, val in sorted(params.items(), key=lambda x: -len(x[0])):
        expr = re.sub(
            r"\b" + re.escape(name) + r"\b",
            str(val),
            expr,
        )
    return int(eval(expr))  # noqa: S307


def resolve_ports(ports, params):
    """Resolve parameterized port declarations."""
    result = {}
    for name, (direction, width_expr, unpacked_expr) in ports.items():
        width = eval_expr(width_expr, params)
        unpacked = eval_expr(unpacked_expr, params) if unpacked_expr else 1
        result[name] = (direction, width * unpacked)
    return result


def group_chisel_ports(ports):
    """Group Chisel's flattened ports back into base names.

    io_ins_down_0, io_ins_down_1, ... -> io_ins_down with total bits summed.
    """
    groups = {}
    for name, (direction, bits) in ports.items():
        m = re.match(r"^(.+?)_(\d+)$", name)
        if m:
            base = m.group(1)
        else:
            base = name

        if base not in groups:
            groups[base] = {"direction": direction, "width": bits, "count": 0}
        groups[base]["count"] += 1

    result = {}
    for base, info in groups.items():
        result[base] = (info["direction"], info["width"] * info["count"])
    return result


def compare_modules(sv_ports, chisel_ports, module_name):
    """Compare two normalized port dicts. Returns list of error strings."""
    errors = []
    sv_names = set(sv_ports.keys())
    ch_names = set(chisel_ports.keys())

    # The SV version adds a reset port that the Chisel version omits
    # (Chisel's implicit reset is not exposed as an explicit port).
    only_sv = sv_names - ch_names - {"reset"}
    only_ch = ch_names - sv_names
    if only_sv:
        errors.append(f"{module_name}: ports only in SV: {sorted(only_sv)}")
    if only_ch:
        errors.append(f"{module_name}: ports only in Chisel: " f"{sorted(only_ch)}")

    for name in sorted(sv_names & ch_names):
        sv_dir, sv_bits = sv_ports[name]
        ch_dir, ch_bits = chisel_ports[name]
        if sv_dir != ch_dir:
            errors.append(
                f"{module_name}.{name}: direction " f"SV={sv_dir} Chisel={ch_dir}"
            )
        if sv_bits != ch_bits:
            # Chisel removes unused io_lsbIns[0], so
            # SV has COLS bits vs Chisel's COLS-1.
            if name == "io_lsbIns" and sv_bits == ch_bits + 1:
                continue
            errors.append(
                f"{module_name}.{name}: " f"width SV={sv_bits} Chisel={ch_bits}"
            )

    return errors


def main():
    if len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <sv_file> <chisel_file> <rows> <cols>")
        sys.exit(1)

    sv_file = sys.argv[1]
    chisel_file = sys.argv[2]
    rows, cols = int(sys.argv[3]), int(sys.argv[4])

    with open(sv_file) as f:
        sv_text = f.read()
    with open(chisel_file) as f:
        chisel_text = f.read()

    errors = []

    # Compare MockArray
    sv_raw = parse_ports(sv_text, "MockArray")
    ch_raw = parse_ports(chisel_text, "MockArray")
    if sv_raw is None:
        errors.append("MockArray not found in SV file")
    elif ch_raw is None:
        errors.append("MockArray not found in Chisel file")
    else:
        params = {
            "WIDTH": cols,
            "HEIGHT": rows,
            "DATA_WIDTH": 64,
        }
        sv_mock = resolve_ports(sv_raw, params)
        ch_mock = group_chisel_ports(resolve_ports(ch_raw, {}))
        errors.extend(compare_modules(sv_mock, ch_mock, "MockArray"))

    # Compare Element
    sv_raw = parse_ports(sv_text, "Element")
    ch_raw = parse_ports(chisel_text, "Element")
    if sv_raw is None:
        errors.append("Element not found in SV file")
    elif ch_raw is None:
        errors.append("Element not found in Chisel file")
    else:
        sv_elem = resolve_ports(
            sv_raw,
            {"ELEMENT_COLS": cols},
        )
        ch_elem = group_chisel_ports(resolve_ports(ch_raw, {}))
        errors.extend(compare_modules(sv_elem, ch_elem, "Element"))

    if errors:
        print("Interface mismatch between SystemVerilog and Chisel:")
        for e in errors:
            print(f"  {e}")
        sys.exit(1)

    print(f"OK: MockArray and Element interfaces match ({rows}x{cols})")


if __name__ == "__main__":
    main()
