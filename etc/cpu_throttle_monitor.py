#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Monitor OpenROAD CPU throttle slot utilization.

Reads /proc/locks to show which CPU slots are held and by which processes,
without interfering with the lock files. Linux only.
"""

import argparse
import json
import os
import sys
import time
from collections import defaultdict
from datetime import datetime
from pathlib import Path

SEM_DIR = Path("/tmp/openroad_cpu_sem")


def build_inode_map():
    """Map (maj:min:inode) -> filename for all lock files in SEM_DIR."""
    inode_map = {}
    total_cpus = 0
    if not SEM_DIR.exists():
        return inode_map, total_cpus
    for entry in SEM_DIR.iterdir():
        try:
            st = entry.stat()
        except OSError:
            continue
        major = os.major(st.st_dev)
        minor = os.minor(st.st_dev)
        key = f"{major:02x}:{minor:02x}:{st.st_ino}"
        inode_map[key] = entry.name
        if entry.name.startswith("cpu."):
            total_cpus += 1
    return inode_map, total_cpus


def parse_proc_locks(inode_map):
    """Parse /proc/locks and return {filename: pid} for our lock files."""
    holders = {}
    try:
        with open("/proc/locks") as f:
            for line in f:
                parts = line.split()
                # Format: ID: TYPE MODE RW PID MAJ:MIN:INODE START END
                if len(parts) < 8:
                    continue
                if parts[1] != "FLOCK":
                    continue
                pid = int(parts[4])
                dev_inode = parts[5]
                if dev_inode in inode_map:
                    holders[inode_map[dev_inode]] = pid
    except (OSError, PermissionError) as e:
        print(f"Error reading /proc/locks: {e}", file=sys.stderr)
    return holders


def get_process_info(pid):
    """Read process name and command line from /proc/<pid>/."""
    try:
        comm = Path(f"/proc/{pid}/comm").read_text().strip()
    except (OSError, PermissionError):
        comm = "(unknown)"
    try:
        cmdline = Path(f"/proc/{pid}/cmdline").read_text().replace("\0", " ").strip()
    except (OSError, PermissionError):
        cmdline = ""
    return comm, cmdline


def collect_status():
    """Collect current throttle status. Returns a dict."""
    inode_map, total_cpus = build_inode_map()
    if not inode_map:
        return {
            "total_cpus": 0,
            "slots_held": 0,
            "slots_free": 0,
            "coordinator_locked": False,
            "processes": {},
            "slot_details": {},
        }

    holders = parse_proc_locks(inode_map)

    # Separate CPU slots from coordinator
    coordinator_locked = "coordinator.lock" in holders
    cpu_holders = {k: v for k, v in holders.items() if k.startswith("cpu.")}

    # Group by PID
    pid_slots = defaultdict(list)
    for slot_name, pid in cpu_holders.items():
        pid_slots[pid].append(slot_name)

    processes = {}
    for pid, slots in sorted(pid_slots.items()):
        comm, cmdline = get_process_info(pid)
        processes[pid] = {
            "comm": comm,
            "cmdline": cmdline,
            "slots": sorted(slots, key=lambda s: int(s.split(".")[1])),
            "count": len(slots),
        }

    slots_held = len(cpu_holders)
    return {
        "total_cpus": total_cpus,
        "slots_held": slots_held,
        "slots_free": total_cpus - slots_held,
        "coordinator_locked": coordinator_locked,
        "processes": processes,
        "slot_details": {
            k: v
            for k, v in sorted(
                cpu_holders.items(), key=lambda x: int(x[0].split(".")[1])
            )
        },
    }


def format_bar(held, total, width=50):
    """Render a progress bar."""
    if total == 0:
        return "[" + " " * width + "]  0/0"
    filled = int(width * held / total)
    bar = "\u2588" * filled + " " * (width - filled)
    pct = 100 * held / total
    return f"[{bar}]  {held}/{total}  ({pct:.0f}%)"


def print_snapshot(status, verbose=False):
    """Print a one-shot status table."""
    total = status["total_cpus"]
    held = status["slots_held"]
    free = status["slots_free"]

    if total == 0:
        print("CPU Throttle: no lock files found in /tmp/openroad_cpu_sem/")
        print("(throttle may not have been used yet)")
        return

    print(f"CPU Throttle Status  ({total} cores)")
    print("=" * 40)
    print(format_bar(held, total))
    print()

    if status["coordinator_locked"]:
        print("** ALLOCATION IN PROGRESS **")
        print()

    if not status["processes"]:
        print("No processes currently holding slots.")
        return

    print(f"{'PID':<8} {'CMD':<20} {'SLOTS':>5}")
    print(f"{'------':<8} {'-------------------':<20} {'-----':>5}")
    for pid, info in sorted(status["processes"].items(), key=lambda x: -x[1]["count"]):
        print(f"{pid:<8} {info['comm']:<20} {info['count']:>5}")

    if verbose:
        print()
        print("Slot details:")
        for slot_name, pid in sorted(
            status["slot_details"].items(), key=lambda x: int(x[0].split(".")[1])
        ):
            comm = status["processes"].get(pid, {}).get("comm", "?")
            print(f"  {slot_name:<10} -> PID {pid} ({comm})")


def print_watch(status):
    """Print watch-mode output with ANSI clear."""
    sys.stdout.write("\033[H\033[J")  # Clear screen
    total = status["total_cpus"]
    held = status["slots_held"]

    if total == 0:
        print("CPU Throttle: waiting for /tmp/openroad_cpu_sem/ ...")
        print(f"\nUpdated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        return

    print(format_bar(held, total))
    print()

    if status["coordinator_locked"]:
        print("** ALLOCATION IN PROGRESS **")
        print()

    if status["processes"]:
        print(f"{'PID':<8} {'CMD':<20} {'SLOTS':>5}")
        print(f"{'------':<8} {'-------------------':<20} {'-----':>5}")
        for pid, info in sorted(
            status["processes"].items(), key=lambda x: -x[1]["count"]
        ):
            print(f"{pid:<8} {info['comm']:<20} {info['count']:>5}")
    else:
        print("No processes currently holding slots.")

    print(f"\nUpdated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")


def append_history(status, history_file):
    """Append a CSV line to the history file."""
    ts = time.time()
    num_procs = len(status["processes"])
    coord = "true" if status["coordinator_locked"] else "false"
    line = (
        f"{ts:.3f},{status['slots_held']},{status['total_cpus']},"
        f"{num_procs},{coord}\n"
    )
    with open(history_file, "a") as f:
        if f.tell() == 0 or os.path.getsize(history_file) == 0:
            f.write(
                "timestamp,slots_held,slots_total,num_processes," "coordinator_locked\n"
            )
        f.write(line)


def main():
    parser = argparse.ArgumentParser(
        description="Monitor OpenROAD CPU throttle slot utilization."
    )
    parser.add_argument(
        "-w",
        "--watch",
        type=float,
        metavar="SECONDS",
        help="Refresh every N seconds (live mode)",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Show per-slot details"
    )
    parser.add_argument("--json", action="store_true", help="Output as JSON")
    parser.add_argument(
        "--history", metavar="FILE", help="Append timestamped CSV records to FILE"
    )
    args = parser.parse_args()

    if sys.platform != "linux":
        print(
            "Error: CPU throttle monitoring requires Linux (/proc/locks).",
            file=sys.stderr,
        )
        sys.exit(1)

    if args.watch is not None:
        try:
            while True:
                status = collect_status()
                if args.json:
                    sys.stdout.write("\033[H\033[J")
                    print(json.dumps(status, indent=2, default=str))
                else:
                    print_watch(status)
                if args.history:
                    append_history(status, args.history)
                time.sleep(args.watch)
        except KeyboardInterrupt:
            print("\nStopped.")
    else:
        status = collect_status()
        if args.json:
            print(json.dumps(status, indent=2, default=str))
        else:
            print_snapshot(status, verbose=args.verbose)
        if args.history:
            append_history(status, args.history)


if __name__ == "__main__":
    main()
