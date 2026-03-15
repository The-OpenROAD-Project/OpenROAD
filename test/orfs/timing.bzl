"""Timing report generation for ORFS flows.

Usage:
    load("//test/orfs:timing.bzl", "orfs_timing_stages")

    orfs_timing_stages(
        name = "gcd",
        stages = ["synth", "floorplan", "place", "cts", "grt", "route"],
        timing_script = "//etc:timing_report",
    )

    bazelisk build //test/orfs/gcd:gcd_synth_timing
    bazelisk run //test/orfs/gcd:gcd_synth_timing
"""

load("@bazel-orfs//:openroad.bzl", "OrfsInfo", "PdkInfo")

# Stage name -> (ODB output group, SDC output group)
# buildifier: disable=unsorted-dict-items
TIMING_STAGE_OUTPUTS = {
    "synth": ("1_synth.odb", "1_synth.sdc"),
    "floorplan": ("2_floorplan.odb", "2_floorplan.sdc"),
    "place": ("3_place.odb", "3_place.sdc"),
    "cts": ("4_cts.odb", "4_cts.sdc"),
    "grt": ("5_1_grt.odb", "5_1_grt.sdc"),
    "route": ("5_route.odb", "5_route.sdc"),
}

def _timing_gen_impl(ctx):
    stage = ctx.attr.stage
    odb_group = ctx.attr.odb_group
    sdc_group = ctx.attr.sdc_group

    # Get ODB and SDC files from output groups
    odb_files = stage[OutputGroupInfo][odb_group].to_list()
    sdc_files = stage[OutputGroupInfo][sdc_group].to_list()
    if not odb_files or not sdc_files:
        fail("Could not find ODB or SDC in output groups")
    odb_file = odb_files[0]
    sdc_file = sdc_files[0]

    # Get liberty files from PdkInfo, filtered to NLDM FF corner
    # (matches what ORFS synthesis uses for timing).
    # PdkInfo.libs contains ALL corners — loading all would OOM.
    lib_files = [
        f
        for f in stage[PdkInfo].libs.to_list()
        if "NLDM" in f.path and "_FF_" in f.path
    ]

    # Get additional libs from OrfsInfo (macro abstracts)
    additional_libs = stage[OrfsInfo].additional_libs.to_list()
    all_libs = lib_files + additional_libs

    html_out = ctx.actions.declare_file(ctx.attr.name + ".html")
    md_out = ctx.actions.declare_file(ctx.attr.name + ".md")

    lib_paths = " ".join([f.path for f in all_libs])

    ctx.actions.run_shell(
        outputs = [html_out, md_out],
        inputs = [odb_file, sdc_file] + all_libs,
        tools = [ctx.executable.timing_script],
        command = " ".join([
            "ODB_FILE={odb}".format(odb = odb_file.path),
            "SDC_FILE={sdc}".format(sdc = sdc_file.path),
            "LIB_FILES='{libs}'".format(libs = lib_paths),
            "DESIGN_NAME={design}".format(design = ctx.attr.design),
            "PLATFORM={platform}".format(platform = stage[PdkInfo].name),
            "REPORTS_DIR=$PWD/{outdir}".format(outdir = html_out.dirname),
            ctx.executable.timing_script.path,
            "&& mv $PWD/{outdir}/1_timing.html {html}".format(
                outdir = html_out.dirname,
                html = html_out.path,
            ),
            "&& mv $PWD/{outdir}/1_timing.md {md}".format(
                outdir = md_out.dirname,
                md = md_out.path,
            ),
        ]),
    )

    return [DefaultInfo(files = depset([html_out, md_out]))]

_timing_gen = rule(
    implementation = _timing_gen_impl,
    attrs = {
        "design": attr.string(default = "unknown"),
        "odb_group": attr.string(mandatory = True),
        "sdc_group": attr.string(mandatory = True),
        "stage": attr.label(
            mandatory = True,
            providers = [OutputGroupInfo, PdkInfo, OrfsInfo],
        ),
        "timing_script": attr.label(
            mandatory = True,
            executable = True,
            cfg = "exec",
        ),
    },
)

def orfs_timing(
        name,
        stage,
        odb_group,
        sdc_group,
        design = None,
        timing_script = None,
        tags = ["manual"]):
    """Generate timing reports from an ORFS stage.

    Args:
        name: Target name (e.g., "gcd_synth_timing")
        stage: Label of the ORFS stage target
        odb_group: Output group name for the ODB file
        sdc_group: Output group name for the SDC file
        design: Design name for report header
        timing_script: Label of the timing_report.py py_binary
        tags: Build tags
    """
    if design == None:
        design = name.rsplit("_timing", 1)[0]

    if timing_script == None:
        return

    _timing_gen(
        name = name + "_gen",
        stage = stage,
        odb_group = odb_group,
        sdc_group = sdc_group,
        design = design,
        timing_script = timing_script,
        tags = tags,
    )

    native.genrule(
        name = name + "_launcher",
        srcs = [],
        outs = [name + "_launch.sh"],
        cmd = """echo '#!/bin/bash' > $@ && echo 'HTML=$$(find "$$(dirname $$0)" -name "{name}_gen.html" | head -1) && echo "$$HTML" && xdg-open "$$HTML" 2>/dev/null || open "$$HTML" 2>/dev/null || echo "Open: $$HTML"' >> $@""".format(name = name),
        tags = tags,
    )

    native.sh_binary(
        name = name,
        srcs = [":" + name + "_launcher"],
        data = [":" + name + "_gen"],
        tags = tags,
    )

def orfs_timing_stages(name, stages, timing_script, variant = None, tags = ["manual"]):
    """Create timing report targets for multiple ORFS stages.

    Args:
        name: Base module name (e.g., "gcd")
        stages: List of stage names (e.g., ["synth", "floorplan", "place", "cts", "grt", "route"])
        timing_script: Label of timing_report.py py_binary
        variant: Variant string or None
        tags: Build tags
    """
    for stage_name in stages:
        if stage_name not in TIMING_STAGE_OUTPUTS:
            continue
        odb_group, sdc_group = TIMING_STAGE_OUTPUTS[stage_name]
        step = name + ("_" + variant if variant else "") + "_" + stage_name
        orfs_timing(
            name = step + "_timing",
            stage = ":" + step,
            odb_group = odb_group,
            sdc_group = sdc_group,
            design = name,
            timing_script = timing_script,
            tags = tags,
        )
