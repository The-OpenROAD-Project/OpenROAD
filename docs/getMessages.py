#!/usr/bin/env python3

import os

command = "python ../etc/find_messages.py -d ../src"
output = os.popen(command).read()

with open('user/MessagesFinal.md', 'w') as f:
    f.write("# OpenROAD Messages Glossary\n")
    f.write("Listed below are the OpenROAD warning/errors you may encounter while using the application.\n")
    f.write("\n")
    f.write("| Tool | Code | Filename:Line Number | Type | Information             |\n")
    f.write("| ---- | ---- | -------------------- | ---- | ----------------------- |\n")

    lines = output.split('\n')
    for line in lines:
        columns = line.split()
        if not columns: continue
        ant = columns[0] 
        num = columns[1]
        fileLineNum = f"[{columns[2]}]({columns[-1]})"
        msgType = columns[-2]
        tool = columns[0].lower()
        try:
            message = open(f"../src/{tool}/doc/messages/{num}.md").read().strip()
        except OSError as e:
            message = "-"
        if not message: message = "-"
        f.write(f"| {ant} | {num} | {fileLineNum} | {msgType} |{message} |\n")
