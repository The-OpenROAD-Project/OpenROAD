"""Timing report generation for ORFS flows.

Usage:
    load("//test/orfs:timing.bzl", "orfs_timing_stages")

    orfs_timing_stages(
        name = "gcd",
        stages = ["synth", "floorplan", "place", "cts", "grt", "route"],
        timing_script = "//etc:timing_report.py",
    )

    bazelisk build //test/orfs/gcd:gcd_synth_timing
    bazelisk run //test/orfs/gcd:gcd_synth_timing
"""

# Stage name -> (ODB output group, SDC output group)
TIMING_STAGE_OUTPUTS = {
    "synth": ("1_synth.odb", "1_synth.sdc"),
    "floorplan": ("2_floorplan.odb", "2_floorplan.sdc"),
    "place": ("3_place.odb", "3_place.sdc"),
    "cts": ("4_cts.odb", "4_cts.sdc"),
    "grt": ("5_1_grt.odb", "5_1_grt.sdc"),
    "route": ("5_route.odb", "5_route.sdc"),
}

def orfs_timing(
        name,
        stage,
        odb_group,
        sdc_group,
        design = None,
        openroad = "//:openroad",
        timing_script = None,
        tags = ["manual"]):
    """Generate timing reports from an ORFS stage.

    Args:
        name: Target name (e.g., "gcd_synth_timing")
        stage: Label of the ORFS stage target
        odb_group: Output group name for the ODB file
        sdc_group: Output group name for the SDC file
        design: Design name for report header
        openroad: Label of the openroad binary
        timing_script: Label of the timing_report.py script
        tags: Build tags
    """
    if design == None:
        design = name.rsplit("_timing", 1)[0]

    if timing_script == None:
        return

    native.filegroup(
        name = name + "_odb",
        srcs = [stage],
        output_group = odb_group,
        tags = tags,
    )

    native.filegroup(
        name = name + "_sdc",
        srcs = [stage],
        output_group = sdc_group,
        tags = tags,
    )

    native.genrule(
        name = name + "_gen",
        srcs = [
            ":" + name + "_odb",
            ":" + name + "_sdc",
        ],
        outs = [
            name + ".html",
            name + ".md",
        ],
        cmd = " ".join([
            "ODB_FILE=$$(realpath $(location :{name}_odb))".format(name = name),
            "SDC_FILE=$$(realpath $(location :{name}_sdc))".format(name = name),
            "DESIGN_NAME={design}".format(design = design),
            "PLATFORM=unknown",
            "REPORTS_DIR=$$PWD/$(@D)",
            "$(location {openroad}) -no_init -python".format(openroad = openroad),
            "$(location {timing_script})".format(timing_script = timing_script),
            "&& mv $$PWD/$(@D)/1_timing.html $(location {name}.html)".format(name = name),
            "&& mv $$PWD/$(@D)/1_timing.md $(location {name}.md)".format(name = name),
        ]),
        tags = tags,
        tools = [
            openroad,
            timing_script,
        ],
    )

    native.genrule(
        name = name + "_launcher",
        srcs = [],
        outs = [name + "_launch.sh"],
        cmd = """echo '#!/bin/bash' > $@ && echo 'HTML=$$(find "$$(dirname $$0)" -name "{name}.html" | head -1) && echo "$$HTML" && xdg-open "$$HTML" 2>/dev/null || open "$$HTML" 2>/dev/null || echo "Open: $$HTML"' >> $@""".format(name = name),
        tags = tags,
    )

    native.sh_binary(
        name = name,
        srcs = [":" + name + "_launcher"],
        data = [":" + name + "_gen"],
        tags = tags,
    )

def orfs_timing_stages(name, stages, timing_script, variant = None, openroad = "//:openroad", tags = ["manual"]):
    """Create timing report targets for multiple ORFS stages.

    Args:
        name: Base module name (e.g., "gcd")
        stages: List of stage names (e.g., ["synth", "floorplan", "place", "cts", "grt", "route"])
        timing_script: Label of timing_report.py
        variant: Variant string or None
        openroad: Label of the openroad binary
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
            openroad = openroad,
            timing_script = timing_script,
            tags = tags,
        )
