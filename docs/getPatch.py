#!/usr/bin/env python3
import os


def swap_prefix(file, old, new):
    with open(file, "r") as f:
        lines = f.read()
    lines = lines.replace(old, new)
    with open(file, "wt") as f:
        f.write(lines)


# modify ../include/ord/OpenROAD.hh
swap_prefix(
    "../include/ord/OpenRoad.hh",
    "namespace dft {\nclass Dft;\n}",
    "namespace dft {\nclass Dft;\n}\n\nnamespace tool{\nclass Tool;\n}",
)

swap_prefix(
    "../include/ord/OpenRoad.hh",
    "dft::Dft* getDft() { return dft_; }",
    "dft::Dft* getDft() { return dft_; }\n  tool::Tool* getTool() { return tool_; }",
)

swap_prefix(
    "../include/ord/OpenRoad.hh",
    "dft::Dft* dft_ = nullptr;",
    "dft::Dft* dft_ = nullptr;\n  tool::Tool* tool_ = nullptr;",
)

# modify ../src/CMakeLists.txt
swap_prefix(
    "../src/CMakeLists.txt",
    "add_subdirectory(dft)",
    "add_subdirectory(dft)\nadd_subdirectory(tool)",
)

swap_prefix("../src/CMakeLists.txt", "pdn\n  dft", "pdn\n  dft\n  tool")

# modify ../src/OpenROAD.cc
swap_prefix(
    "../src/OpenRoad.cc",
    '#include "utl/MakeLogger.h"',
    '#include "utl/MakeLogger.h"\n#include "tool/MakeTool.hh"',
)

swap_prefix(
    "../src/OpenRoad.cc",
    "dft_ = dft::makeDft();",
    "dft_ = dft::makeDft();\n  tool_ = makeTool();",
)

swap_prefix(
    "../src/OpenRoad.cc",
    "dft::deleteDft(dft_);",
    "dft::deleteDft(dft_);\n  deleteTool(tool_);",
)

swap_prefix(
    "../src/OpenRoad.cc",
    "dft::initDft(this);",
    "dft::initDft(this);\n  initTool(this);",
)

# create a patch file
_ = os.popen("git add ../include/ord/OpenRoad.hh").read()
_ = os.popen("git add ../src/CMakeLists.txt").read()
_ = os.popen("git add ../src/OpenRoad.cc").read()
_ = os.popen("git diff --cached > misc/AddTool.patch").read()

# restore all changes except patch
_ = os.popen("git reset ../include/ord/OpenRoad.hh").read()
_ = os.popen("git reset ../src/CMakeLists.txt").read()
_ = os.popen("git reset ../src/OpenRoad.cc").read()
_ = os.popen("git restore ../include/ord/OpenRoad.hh").read()
_ = os.popen("git restore ../src/CMakeLists.txt").read()
_ = os.popen("git restore ../src/OpenRoad.cc").read()
