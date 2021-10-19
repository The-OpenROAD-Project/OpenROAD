#!/usr/bin/env python3

import os
newPath = '(../'
prefix = '(./main/'
with open('index.md', 'r') as f:
    lines = f.read()
lines = lines.replace(prefix, newPath)
with open('index.md', 'wt') as f:
    f.write(lines)
