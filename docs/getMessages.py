#!/usr/bin/env python3

import re
import subprocess

# Decode the message list as UTF-8 rather than in whatever encoding the
# ambient locale selects; find_messages.py writes it as UTF-8. stderr is left
# alone so its duplicate-id diagnostics stay visible.
command = ["python", "../etc/find_messages.py", "-d", "../src"]
output = subprocess.run(command, stdout=subprocess.PIPE, encoding="utf-8").stdout

with open("user/MessagesFinal.md", "w", encoding="utf-8") as f:
    f.write("# OpenROAD Messages Glossary\n")
    f.write(
        "Listed below are the OpenROAD warning/errors you may encounter while using the application.\n"
    )
    f.write("\n")
    f.write("| Tool | Code | Filename:Line Number | Type | Information             |\n")
    f.write("| ---- | ---- | -------------------- | ---- | ----------------------- |\n")

    lines = output.split("\n")
    for line in lines:
        columns = line.split()
        if not columns:
            continue
        ant = columns[0]
        num = columns[1]
        fileLineNum = f"[{columns[2]}]({columns[-1]})"
        msgType = columns[-2]
        tool = columns[0].lower()
        try:
            # aim is to match all level1 header and their corresponding text.
            message = open(
                f"../src/{tool}/doc/messages/{num}.md", encoding="utf-8"
            ).read()
            pattern = re.compile(
                r"#\s*(?P<header1>[^\n]+)\n*(?P<body_text>.*?)(?=\n#|$)", re.DOTALL
            )
            matches = pattern.finditer(message)
            m = []
            for match in matches:
                header1 = match.group("header1")
                body_text = match.group("body_text").strip()
                m.append(f"{header1}-{body_text}")
            message = " ".join(x for x in m)

        except OSError as e:
            message = "-"
        if not message:
            message = "-"
        f.write(f"| {ant} | {num} | {fileLineNum} | {msgType} |{message} |\n")
