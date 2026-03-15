# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

"""Rule to build OpenROAD man pages from markdown sources."""

def _man_pages_impl(ctx):
    # Declare output directories as tree artifacts
    cat_dir = ctx.actions.declare_directory("cat")
    html_dir = ctx.actions.declare_directory("html")

    # Build a map of script basename -> path for easy lookup
    scripts_by_name = {f.basename: f.path for f in ctx.files.scripts}

    # Validate all required scripts are present
    required_scripts = [
        "extract_utils.py",
        "link_readmes.sh",
        "manpage.py",
        "md_roff_compat.py",
    ]
    for script in required_scripts:
        if script not in scripts_by_name:
            fail("Required script '{}' is missing from the 'scripts' attribute".format(script))

    all_inputs = (
        ctx.files.man1_srcs +
        ctx.files.man2_srcs +
        ctx.files.scripts +
        [ctx.file.makefile]
    )

    ctx.actions.run_shell(
        inputs = all_inputs,
        outputs = [cat_dir, html_dir],
        command = """
set -euo pipefail

WORK_DIR=$(mktemp -d)
trap "rm -rf $WORK_DIR" EXIT
EXECROOT=$(pwd)

# Set up docs directory structure
mkdir -p "$WORK_DIR/md/man1"
mkdir -p "$WORK_DIR/md/man2"
mkdir -p "$WORK_DIR/md/man3"
mkdir -p "$WORK_DIR/src/scripts"

# Copy scripts and Makefile
cp {link_readmes} "$WORK_DIR/src/scripts/link_readmes.sh"
cp {md_roff_compat} "$WORK_DIR/src/scripts/md_roff_compat.py"
cp {manpage} "$WORK_DIR/src/scripts/manpage.py"
cp {extract_utils} "$WORK_DIR/src/scripts/extract_utils.py"
cp {makefile} "$WORK_DIR/Makefile"

# Copy man1 sources
for f in {man1_srcs}; do
    cp "$f" "$WORK_DIR/md/man1/"
done

# Copy man2 README sources (named by module directory)
for f in {man2_srcs}; do
    module=$(basename $(dirname $f))
    cp "$f" "$WORK_DIR/md/man2/${{module}}.md"
done

# Run the Python preprocessor to generate per-function man2/man3 pages
cd "$WORK_DIR"
python3 src/scripts/md_roff_compat.py || true

# Remove any malformed output files (e.g. '#.md' from READMEs with bad function names)
find "$WORK_DIR/md" -name '#*.md' -delete 2>/dev/null || true

# Build all man page formats; use -k to continue past nroff warnings
make all -k -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) || true

# Ensure output subdirs exist even if some pages failed to build
mkdir -p "$WORK_DIR/cat/cat1" "$WORK_DIR/cat/cat2" "$WORK_DIR/cat/cat3"
mkdir -p "$WORK_DIR/html/html1" "$WORK_DIR/html/html2" "$WORK_DIR/html/html3"

# Copy outputs into the Bazel-declared output directories (paths are execroot-relative)
mkdir -p "$EXECROOT/{cat_out}" "$EXECROOT/{html_out}"
cp -r "$WORK_DIR/cat/." "$EXECROOT/{cat_out}/"
cp -r "$WORK_DIR/html/." "$EXECROOT/{html_out}/"
""".format(
            link_readmes = scripts_by_name["link_readmes.sh"],
            md_roff_compat = scripts_by_name["md_roff_compat.py"],
            manpage = scripts_by_name["manpage.py"],
            extract_utils = scripts_by_name["extract_utils.py"],
            makefile = ctx.file.makefile.path,
            man1_srcs = " ".join([f.path for f in ctx.files.man1_srcs]),
            man2_srcs = " ".join([f.path for f in ctx.files.man2_srcs]),
            cat_out = cat_dir.path,
            html_out = html_dir.path,
        ),
        mnemonic = "BuildManPages",
        progress_message = "Building OpenROAD man pages",
        # pandoc and nroff are system tools not available in the Bazel sandbox
        execution_requirements = {
            "local": "1",
            "no-sandbox": "1",
        },
    )

    return [DefaultInfo(files = depset([cat_dir, html_dir]))]

man_pages = rule(
    implementation = _man_pages_impl,
    attrs = {
        "makefile": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
        "man1_srcs": attr.label_list(
            allow_files = [".md"],
            doc = "man1 markdown source files",
        ),
        "man2_srcs": attr.label_list(
            allow_files = [".md"],
            doc = "man2 README.md source files (one per module)",
        ),
        "scripts": attr.label_list(
            allow_files = True,
            doc = "Build scripts needed to generate man pages",
        ),
    },
)
