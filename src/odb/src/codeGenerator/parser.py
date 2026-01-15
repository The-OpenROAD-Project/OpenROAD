# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

import re
from typing import List, Dict, Optional


def _find_index(lines: List[str], target: str, start_index: int = 0) -> int:
    """Find the index of target in lines[start_index:] ignoring white space"""
    for i in range(start_index, len(lines)):
        if lines[i].strip().replace(" ", "") == target:
            return i
    return -1


def _get_sections(
    lines: List[str],
    tag: str,
    sections: Optional[Dict[str, List[str]]] = None,
    remove: bool = False,
) -> None:
    """Find all sections delimeted by begin/end tags in lines.

    The lines containing the section tags are kept and not stored or deleted.
    Only the lines within by the section tags are operated on.

    If remove=False the lines found are stored in sections by tag name.
       Empty sections are not stored.
    If remove=True the lines found are deleted from lines
    """
    if sections is None:
        sections = {}

    i = 0
    while i < len(lines):
        line = lines[i].strip().replace(" ", "")
        if line.startswith(tag):
            name = line[len(tag) :]
            if name in sections and not remove:
                raise Exception(f"Duplicate Tag: {name}")

            end_tag = line.replace("Begin", "End", 1)
            end = _find_index(lines, end_tag, i)
            if end == -1:
                raise Exception(f"Could not find an End for tag {name}\n")

            if remove:
                del lines[i + 1 : end]
            else:
                sections.setdefault(name, []).extend(lines[i + 1 : end])
        i += 1


class Parser:
    """Parses a file looking for the sections delimited by the code_tags."""

    def __init__(self, file_name: str):
        with open(file_name, "r", encoding="ascii") as file:
            self.lines = file.readlines()
        self.generator_code: Dict[str, List[str]] = {}
        self.user_code: Dict[str, List[str]] = {}
        self.set_comment_str("//")

    def set_comment_str(self, comment: str) -> None:
        """Set the prefix for the code tags comments

        For example, CMakeLists.txt uses # rather than //"""
        self.user_code_tag = f"{comment}UserCodeBegin"
        self.generator_code_tag = f"{comment}GeneratorCodeBegin"

    def parse_user_code(self) -> None:
        """Parse using the user_code_tag

        Used for a file where the default is generated code and user
        code is tagged."""
        _get_sections(self.lines, self.user_code_tag, self.user_code)

    def parse_source_code(self, file_name: str) -> None:
        """Parse using the file using the generator_code_tag

        Used for a file where the default is user code and generated
        code is tagged."""
        with open(file_name, "r", encoding="ascii") as source_file:
            db_lines = source_file.readlines()
        _get_sections(db_lines, self.generator_code_tag, self.generator_code)

    def clean_code(self) -> None:
        """Remove all section lines using the generator_code_tag

        Empties the generated sections in preparation for new code"""
        _get_sections(self.lines, self.generator_code_tag, remove=True)

    def write_in_file(self, file_name: str, keep_empty: bool) -> None:
        """Write the file with user and/or generated sections."""
        # Replace the user sections inside the generate sections with
        # their current contents.
        for section, db_lines in self.generator_code.items():
            j = 0
            while j < len(db_lines):
                line = db_lines[j].strip().replace(" ", "")
                if line.startswith(self.user_code_tag):
                    name = line[len(self.user_code_tag) :]
                    user = self.user_code.pop(name, [])

                    if user or keep_empty:
                        db_lines[j + 1 : j + 1] = user
                    else:
                        del db_lines[j : j + 2]
                        j -= 1  # adjust index after deletion
                j += 1
            self.generator_code[section] = db_lines

        # Ensure all non-empty user tags were used in the new generated
        # code.  We don't want to lose any previous user code accidentally.
        if any(self.user_code.values()):
            tags = [tag for tag, content in self.user_code.items() if content]
            raise Exception(f"User tags {tags} not used in {file_name}")

        # Replace the generated sections with their updated content
        i = 0
        while i < len(self.lines):
            line = self.lines[i].strip().replace(" ", "")
            if line.startswith(self.generator_code_tag):
                name = line[len(self.generator_code_tag) :]
                self.lines[i + 1 : i + 1] = self.generator_code.get(name, [])
            i += 1

        with open(file_name, "w", encoding="ascii") as out:
            out.write("".join(self.lines))
