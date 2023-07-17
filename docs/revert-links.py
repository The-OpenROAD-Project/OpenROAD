#!/usr/bin/env python3

def swap_prefix(file, old, new):
    with open(file, 'r') as f:
        lines = f.read()
    lines = lines.replace(old, new)
    with open(file, 'wt') as f:
        f.write(lines)


swap_prefix('../README.md', '(../', '(docs/')
swap_prefix('../README.md', '```{mermaid}\n:align: center\n', '```mermaid')
