import odb
import os
import utl
import re
from openroad import Design

if os.environ.get("BAZEL_TEST") == "1":
    from openroad import PlaceOptions
else:
    import gpl

    PlaceOptions = gpl.PlaceOptions


def get_runfiles_path_to(path):
    # If we're not in bazel mode assume Ctest deals
    # with our CWD and return "".
    runfiles_path = os.environ.get("TEST_SRCDIR", "")
    if runfiles_path:
        runfiles_path = runfiles_path + path
    return runfiles_path


def if_bazel_change_working_dir_to(test_dir):
    test_dir = get_runfiles_path_to(test_dir)
    if test_dir:
        os.chdir(test_dir)


def make_rect(design, xl, yl, xh, yh):
    xl = design.micronToDBU(xl)
    yl = design.micronToDBU(yl)
    xh = design.micronToDBU(xh)
    yh = design.micronToDBU(yh)
    return odb.Rect(xl, yl, xh, yh)


def make_result_file(filename):
    result_dir = os.path.join(os.getcwd(), "results")
    bazel_tmp_output_dir = os.environ.get("TEST_TMPDIR", "")

    if bazel_tmp_output_dir:
        result_dir = os.path.join(
            bazel_tmp_output_dir,
        )

    if not os.path.exists(result_dir):
        os.mkdir(result_dir)

    root_ext = os.path.splitext(filename)
    filename = "{}-py{}".format(*root_ext)
    return os.path.join(result_dir, filename)


def diff_files(file1, file2, ignore=None):
    if ignore:
        ignore = re.compile(ignore)

    # If we're in bazel use print as the global utl functions
    # have been removed.
    if os.environ.get("TEST_SRCDIR", ""):
        report_function = print
    else:
        report_function = utl.report

    with open(file1, "r") as f:
        lines1 = f.readlines()

    with open(file2, "r") as f:
        lines2 = f.readlines()

    num_lines1 = len(lines1)
    num_lines2 = len(lines2)
    for i in range(min(num_lines1, num_lines2)):
        if ignore and (ignore.search(lines1[i]) or ignore.search(lines2[i])):
            continue
        if lines1[i] != lines2[i]:
            report_function(f"Differences found at line {i+1}.")
            report_function(lines1[i][:-1])
            report_function(lines2[i][:-1])
            if os.environ.get("TEST_SRCDIR", ""):
                raise Exception("Diffs found")
            return 1

    if num_lines1 != num_lines2:
        report_function(f"Number of lines differs {num_lines1} vs {num_lines2}.")
        if os.environ.get("TEST_SRCDIR", ""):
            raise Exception("Diffs found")
        return 1

    report_function("No differences found.")
    return 0


def make_design(tech):
    design = Design(tech)
    logger = design.getLogger()

    # Reading DEF file
    logger.suppressMessage(utl.ODB, 127)
    # Finished DEF file
    logger.suppressMessage(utl.ODB, 134)

    # suppress tap info messages
    logger.suppressMessage(utl.TAP, 100)
    logger.suppressMessage(utl.TAP, 101)

    # suppress par messages with files' names
    logger.suppressMessage(utl.PAR, 6)
    logger.suppressMessage(utl.PAR, 38)

    # suppress ord message with number of threads
    logger.suppressMessage(utl.ORD, 30)

    # suppress grt message with the suggested adjustment
    logger.suppressMessage(utl.GRT, 303)
    logger.suppressMessage(utl.GRT, 704)

    return design
