import glob
import os

from extract_utils import extract_help, extract_proc, extract_tcl_code


def count_commands(module_dir):
    """Count help, proc, and readme commands for a module.

    Args:
        module_dir: Absolute path to the module directory (e.g., .../src/cts).

    Returns:
        Tuple of (help_count, proc_count, readme_count).
    """
    module = os.path.basename(module_dir)

    tcl_files = sorted(glob.glob(os.path.join(module_dir, "src", "*.tcl")))

    # odb special: tcl is in src/swig/tcl/, not src/
    if module == "odb":
        odb_tcl = os.path.join(module_dir, "src", "swig", "tcl", "odb.tcl")
        if os.path.exists(odb_tcl):
            tcl_files.append(odb_tcl)

    help_count = 0
    proc_count = 0
    for tcl_file in tcl_files:
        # pad has 3 Tcls; skip ICeWall and PdnGen
        basename = os.path.basename(tcl_file)
        if "ICeWall" in basename or "PdnGen" in basename:
            continue

        with open(tcl_file, encoding="utf-8") as f:
            content = f.read()
            help_count += len(extract_help(content))
            proc_count += len(extract_proc(content))

    readme_path = os.path.join(module_dir, "README.md")
    readme_count = 0
    if os.path.exists(readme_path):
        with open(readme_path, encoding="utf-8") as f:
            # for gui, filter out gui:: for separate processing
            results = [x for x in extract_tcl_code(f.read()) if "gui::" not in x]
            readme_count = len(results)

    # for pad, remove `make_fake_io_site` because it is a hidden cmd arg
    if module == "pad":
        readme_count -= 1

    return help_count, proc_count, readme_count
