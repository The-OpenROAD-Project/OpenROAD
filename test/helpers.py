import odb
import os
import utl
import re

def make_rect(design, xl, yl, xh, yh):
    xl = design.micronToDBU(xl)
    yl = design.micronToDBU(yl)
    xh = design.micronToDBU(xh)
    yh = design.micronToDBU(yh)
    return odb.Rect(xl, yl, xh, yh)

def make_result_file(filename):
    result_dir = os.path.join(os.getcwd(), 'results')
    if not os.path.exists(result_dir):
        os.mkdir(result_dir)

    root_ext = os.path.splitext(filename)
    filename = "{}-py{}".format(*root_ext)
    return os.path.join(result_dir, filename)

def diff_files(file1, file2, ignore=None):
    if ignore:
        ignore = re.compile(ignore)

    with open(file1, 'r') as f:
        lines1 = f.readlines()

    with open(file2, 'r') as f:
        lines2 = f.readlines()

    num_lines1 = len(lines1)
    num_lines2 = len(lines2)
    for i in range(min(num_lines1, num_lines2)):
        if ignore and (ignore.search(lines1[i]) or ignore.search(lines2[i])):
            continue
        if lines1[i] != lines2[i]:
            utl.report(f"Differences found at line {i+1}.")
            utl.report(lines1[i][:-1])
            utl.report(lines2[i][:-1])
            return 1

    if num_lines1 != num_lines2:
        utl.report(f"Number of lines differs {num_lines1} vs {num_lines2}.")
        return 1

    utl.report("No differences found.")
    return 0

# Output voltage file is specified as ...
utl.suppress_message(utl.PSM, 2)
# Output current file specified ...
utl.suppress_message(utl.PSM, 3)
# Error file is specified as ...
utl.suppress_message(utl.PSM, 83)
# Output spice file is specified as
utl.suppress_message(utl.PSM, 5)
# SPICE file is written at
utl.suppress_message(utl.PSM, 6)
# Reading DEF file
utl.suppress_message(utl.ODB, 127)
# Finished DEF file
utl.suppress_message(utl.ODB, 134)

# suppress PPL info messages
utl.suppress_message(utl.PPL, 41)
utl.suppress_message(utl.PPL, 48)
utl.suppress_message(utl.PPL, 49)
utl.suppress_message(utl.PPL, 60)

# suppress tap info messages
utl.suppress_message(utl.TAP, 100)
utl.suppress_message(utl.TAP, 101)

# suppress par messages with files' names
utl.suppress_message(utl.PAR, 6)
utl.suppress_message(utl.PAR, 38)

# suppress ord message with number of threads
utl.suppress_message(utl.ORD, 30)
