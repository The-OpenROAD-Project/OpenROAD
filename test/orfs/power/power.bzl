"""Minimal power test rules that invoke openroad/sta directly."""

load("@bazel-orfs//private:providers.bzl", "PdkInfo")

def _power_report_impl(ctx):
    pdk = ctx.attr.src[PdkInfo]

    all_libs = sorted(pdk.libs.to_list(), key = lambda f: f.path)
    all_pdk_files = sorted(pdk.files.to_list(), key = lambda f: f.path)

    # Filter liberty files by pattern (e.g. "RVT_FF" for RVT fast-fast corner)
    if ctx.attr.lib_filter:
        libs = [f for f in all_libs if ctx.attr.lib_filter in f.path]
    else:
        libs = all_libs

    lib_paths = " ".join([f.path for f in libs])

    env = {
        "LIB_FILES": lib_paths,
        "OUTPUT": ctx.outputs.out.path,
        "POWER_SDC": ctx.file.sdc.path,
        "POWER_SPEF": ctx.file.spef.path,
        "POWER_VERILOG": ctx.file.verilog.path,
    }

    inputs = [ctx.file.script, ctx.file.verilog, ctx.file.sdc, ctx.file.spef] + libs

    if not ctx.attr.sta:
        # OpenROAD needs LEF files; filter from PDK files
        lefs = [f for f in all_pdk_files if f.path.endswith(".lef")]
        if not lefs:
            fail("No LEF files found in PDK; OpenROAD requires at least a tech LEF")
        env["TECH_LEF"] = lefs[0].path
        if len(lefs) > 1:
            env["SC_LEF"] = " ".join([f.path for f in lefs[1:]])
        inputs += lefs

    tool = ctx.executable.tool

    args = ctx.actions.args()
    args.add("-exit")
    args.add("-no_init")
    args.add("-threads", "max")
    args.add(ctx.file.script)

    ctx.actions.run(
        executable = tool,
        arguments = [args],
        inputs = depset(inputs),
        outputs = [ctx.outputs.out],
        env = env,
        mnemonic = "PowerReport",
    )

    return [DefaultInfo(files = depset([ctx.outputs.out]))]

power_report = rule(
    implementation = _power_report_impl,
    attrs = {
        "lib_filter": attr.string(
            doc = "Substring to filter liberty files (e.g. 'RVT_FF' for RVT fast-fast corner)",
        ),
        "out": attr.output(mandatory = True),
        "script": attr.label(
            allow_single_file = [".tcl"],
            mandatory = True,
        ),
        "sdc": attr.label(allow_single_file = True, mandatory = True),
        "spef": attr.label(allow_single_file = True, mandatory = True),
        "src": attr.label(mandatory = True, providers = [PdkInfo]),
        "sta": attr.bool(default = False),
        "tool": attr.label(
            executable = True,
            cfg = "exec",
            mandatory = True,
        ),
        "verilog": attr.label(allow_single_file = True, mandatory = True),
    },
)
