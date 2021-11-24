#!/usr/bin/env python3

NEW_PATH = '(../'
PREFIX = '(./main/'
with open('index.md', 'r') as f:
    lines = f.read()
lines = lines.replace(PREFIX, NEW_PATH)
with open('index.md', 'wt') as f:
    f.write(lines)
