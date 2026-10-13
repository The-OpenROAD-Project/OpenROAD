#!/usr/bin/env python3

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2026, The OpenROAD Authors

"""Report what a Bazel build actually did: real work versus cache hits.

The console counter ("[7,203 / 18,048] Compiling foo.cxx; 20s disk-cache,
remote-cache, processwrapper-sandbox") advances for cache hits and real
compiles alike.  Build with --config=cachelog and run this script for the
breakdown, including which actions were built for real and why they were not
up to date:

    bazelisk build --config=cachelog //src/dpl:dpl
    etc/bazel-cache-summary.py
"""

import argparse
import json
import os
import re
import sys
from collections import Counter

DEFAULT_LOG = "bazel-cachelog.json"
DEFAULT_EXPLAIN = "bazel-cachelog-explain.txt"

# Runner names Bazel reports for a spawn whose outputs were fetched rather
# than produced.  Every other runner ran the action for real, locally
# (processwrapper-sandbox, linux-sandbox, local, worker) or on a remote
# executor (remote).
CACHE_RUNNERS = ("disk cache hit", "remote cache hit")

EXPLAIN_LINE = re.compile(r"^Executing action '(?P<action>.*)': (?P<reason>.*)$")


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-l",
        "--log",
        default=DEFAULT_LOG,
        help=f"execution log written by --config=cachelog (default: {DEFAULT_LOG})",
    )
    parser.add_argument(
        "-e",
        "--explain",
        default=DEFAULT_EXPLAIN,
        help=f"explanation log written by --config=explain "
        f"(default: {DEFAULT_EXPLAIN})",
    )
    parser.add_argument(
        "-n",
        "--top",
        type=int,
        default=20,
        help="number of slowest executed actions to list (0 for all)",
    )
    return parser.parse_args()


def find_file(path):
    """Look for path as given, then at the root of the enclosing workspace."""
    if os.path.exists(path):
        return path
    if os.path.isabs(path):
        return None
    directory = os.getcwd()
    while True:
        if os.path.exists(os.path.join(directory, "MODULE.bazel")):
            candidate = os.path.join(directory, path)
            return candidate if os.path.exists(candidate) else None
        parent = os.path.dirname(directory)
        if parent == directory:
            return None
        directory = parent


def read_entries(path):
    """Yield the JSON objects concatenated in an execution log.

    The log is a stream of pretty-printed objects rather than one array, and a
    single compile carries thousands of inputs, so decode it incrementally and
    keep at most one record in memory.
    """
    decoder = json.JSONDecoder()
    buffer = ""
    with open(path, encoding="utf-8") as stream:
        while True:
            chunk = stream.read(1 << 20)
            buffer = (buffer + chunk).lstrip()
            while buffer:
                try:
                    entry, end = decoder.raw_decode(buffer)
                except ValueError:
                    break  # Incomplete record, read more.
                yield entry
                buffer = buffer[end:].lstrip()
            if not chunk:
                break
    if buffer:
        raise ValueError(f"trailing garbage in {path}")


def duration_seconds(text):
    """Parse a protobuf JSON duration such as '5.805s'."""
    try:
        return float(text.rstrip("s"))
    except (AttributeError, TypeError, ValueError):
        return 0.0


def describe(entry):
    """Name the action the way the build log names it."""
    args = entry.get("commandArgs", [])
    if entry.get("mnemonic") == "CppCompile" and "-c" in args:
        return args[args.index("-c") + 1]
    for output in entry.get("listedOutputs", []):
        if not output.endswith(".d"):
            return output
    return entry.get("targetLabel", "<unknown>")


def summarize_log(path, top):
    runners = Counter()
    seconds = Counter()
    executed = []
    for entry in read_entries(path):
        runner = entry.get("runner", "<unknown>")
        wall = duration_seconds(entry.get("metrics", {}).get("executionWallTime"))
        runners[runner] += 1
        seconds[runner] += wall
        if runner not in CACHE_RUNNERS:
            executed.append((wall, entry.get("mnemonic", "?"), describe(entry)))

    total = sum(runners.values())
    print(f"{path}: {total:,} spawns")
    if not total:
        return

    for group, wanted in (("ran for real", False), ("served from cache", True)):
        members = [r for r in runners if (r in CACHE_RUNNERS) == wanted]
        if not members:
            continue
        count = sum(runners[r] for r in members)
        wall = sum(seconds[r] for r in members)
        print(f"\n  {group}: {count:,} spawns, {wall:,.1f}s of wall time")
        for runner in sorted(members, key=lambda r: -runners[r]):
            print(f"    {runner:<28} {runners[runner]:>8,}  {seconds[runner]:>9,.1f}s")

    print(
        "\n  Actions served from Bazel's own action cache never reach the\n"
        "  execution log; the build's 'INFO: N processes:' line counts those too."
    )

    if executed and top != 0:
        executed.sort(reverse=True)
        shown = executed if top < 0 else executed[:top]
        print(f"\nSlowest of the {len(executed):,} spawns that ran for real:")
        for wall, mnemonic, name in shown:
            print(f"  {wall:>8.1f}s  {mnemonic:<14} {name}")
        if len(shown) < len(executed):
            print(f"  ... {len(executed) - len(shown):,} more (raise --top)")


def summarize_explain(path):
    reasons = Counter()
    examples = {}
    with open(path, encoding="utf-8") as stream:
        for line in stream:
            match = EXPLAIN_LINE.match(line.rstrip("\n"))
            if match:
                reason = match.group("reason").rstrip(".")
                reasons[reason] += 1
                examples.setdefault(reason, match.group("action"))

    if not reasons:
        return
    print(f"\n{path}: why actions were not up to date")
    for reason, count in reasons.most_common():
        print(f"  {count:>8,}  {reason}")
        print(f"            e.g. {examples[reason]}")


def main():
    args = parse_args()

    log = find_file(args.log)
    explain = find_file(args.explain)
    if not log and not explain:
        sys.exit(
            f"Neither {args.log} nor {args.explain} found. Build with\n"
            "  bazelisk build --config=cachelog <targets>\n"
            "first, or point --log/--explain at the logs."
        )

    if log:
        summarize_log(log, args.top)
    else:
        print(f"{args.log} not found; run with --config=cachelog for spawn detail.")
    if explain:
        summarize_explain(explain)


if __name__ == "__main__":
    main()
