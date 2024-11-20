import os
import re
import argparse
import math
import xlsxwriter
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict


def OpenFile(file_path, rw="r"):
    if rw == "r":
        if os.path.exists(file_path):
            # print(file_path)
            return open(file_path, rw)
        else:
            print(f"The file {file_path} does not exist.")
            exit()
    if rw == "w":
        return open(file_path, rw)


def main():
    arg_parser = argparse.ArgumentParser(
        description="Create Tcl Script to for OpenRCX to generate model files by reading multiple files per process corner"
    )
    arg_parser.add_argument(
        "-in_file",
        type=str,
        default="corners_cap_files",
        help="corners and cap files List",
    )
    arg_parser.add_argument(
        "-out_file",
        type=str,
        default="pattern.caps",
        help="Output Filename, default=pattern.caps",
    )

    args = arg_parser.parse_args()

    outFP = OpenFile(args.out_file, "w")

    # list of files

    corners = defaultdict(list)
    f = OpenFile(args.in_file)
    for line in f:
        if not line.strip():
            continue

        row = line.split()[0].split(":")
        corners[row[0]].append(row[1])
        print(row)

    print(corners)
    for key, value in corners.items():
        for file1 in value:
            print(key, file1)
        print(f"{key}: {value}")


if __name__ == "__main__":
    main()
