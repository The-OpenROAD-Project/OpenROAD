"""Macro to verify two ORFS variants produce identical .odb and .sdc files."""

load("@rules_shell//shell:sh_test.bzl", "sh_test")

STAGES = {
    "cts": "4_cts",
    "final": "6_final",
    "floorplan": "2_floorplan",
    "grt": "5_1_grt",
    "place": "3_place",
    "route": "5_route",
}

def check_same(name, flow, variant_a, variant_b, stages = STAGES, tags = []):
    """Verify two flow variants produce identical .odb and .sdc at every stage.

    Generates one sh_test per stage, each comparing .odb and .sdc files
    between the two variants.

    Args:
        name: Base name for generated test targets
        flow: Base name of the orfs_sweep flow
        variant_a: First variant name
        variant_b: Second variant name
        stages: Dictionary mapping stage short names to output prefixes
        tags: Tags for generated targets
    """
    for stage_short, stage_prefix in stages.items():
        args = []
        data = []
        for ext in ["odb", "sdc"]:
            for variant in [variant_a, variant_b]:
                v_infix = "" if variant == "base" else variant + "_"
                target = flow + "_" + v_infix + stage_short
                fg_name = name + "_" + variant + "_" + stage_short + "_" + ext
                native.filegroup(
                    name = fg_name,
                    srcs = [":" + target],
                    output_group = stage_prefix + "." + ext,
                    tags = tags,
                )
                data.append(":" + fg_name)
                args.append("$(location :" + fg_name + ")")

        sh_test(
            name = name + "_" + stage_short,
            srcs = ["//test/orfs:check_same.sh"],
            args = args,
            data = data,
            tags = tags,
        )
