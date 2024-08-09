#!/usr/bin/env python3


def swap_prefix(file, old, new):
    with open(file, "r") as f:
        lines = f.read()
    lines = lines.replace(old, new)
    with open(file, "wt") as f:
        f.write(lines)


for filename in ["../README.md", "../README2.md"]:
    swap_prefix(filename, "(../", "(docs/")
    swap_prefix(filename, "```{mermaid}\n:align: center\n", "```mermaid")
