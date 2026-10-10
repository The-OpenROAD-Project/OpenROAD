"""Golden-file regressions split into a cached build action and a cheap test.

golden_regression_test(name) creates:

  <name>-tcl_run     Build action: runs openroad on <name>.tcl. Always
                     succeeds; the exit code is recorded as an output.
                     Goldens are not inputs, so it stays cached when a
                     golden changes.
  <name>-tcl_test    Test: compares the log against <name>.ok, every
                     side output against its golden, and the exit code.
  <name>_update      `bazel run` target that copies the cached outputs over
                     the goldens in the source tree (write_source_files).

diff_files in the test's Tcl only records the (result, golden) pair in a
manifest under this rule (see test/helpers.tcl), so the log no longer
embeds the outcome of the side-output comparison and the .ok can be
updated independently of the .defok.

With hash_goldens = True, a side-output golden is checked in as
<golden>.sha256 holding only the digest of the result. Use it for large
outputs where nobody reads the diff. The run outputs only the digest, so
the full result is not cached; the manual <name>-tcl_debug target re-runs
openroad and keeps it, for investigating a mismatch.
"""

load("@bazel_lib//lib:write_source_files.bzl", "write_source_files")
load("//bazel/gpu:defs.bzl", "GPU_ENV_OFF")

def _regression_run_impl(ctx):
    openroad = ctx.attr.openroad[DefaultInfo].files_to_run
    coreutils = ctx.toolchains["@bazel_lib//lib:coreutils_toolchain_type"].coreutils_info.bin
    ctx.actions.run(
        executable = ctx.executable._run_sh,
        arguments = [
            coreutils.path,
            str(ctx.attr.timeout),
            ctx.executable.openroad.path,
            ctx.file.test_file.path,
            ctx.outputs.log.dirname,
            ctx.outputs.log.path,
            ctx.outputs.exit_code.path,
            ctx.outputs.manifest.path,
        ] + [r.path for r in ctx.outputs.results],
        inputs = depset([ctx.file.test_file] + ctx.files.data),
        tools = [openroad, coreutils],
        outputs = [
            ctx.outputs.log,
            ctx.outputs.exit_code,
            ctx.outputs.manifest,
        ] + ctx.outputs.results,
        env = ctx.attr.env,
        mnemonic = "OpenRoadRegression",
        progress_message = "Running OpenROAD regression %{label}",
    )
    return [DefaultInfo(files = depset(
        [ctx.outputs.log, ctx.outputs.exit_code, ctx.outputs.manifest] +
        ctx.outputs.results,
    ))]

regression_run = rule(
    implementation = _regression_run_impl,
    doc = "Run an OpenROAD regression script as a build action.",
    attrs = {
        "data": attr.label_list(
            doc = "Inputs the script reads. Must not include goldens.",
            allow_files = True,
        ),
        "env": attr.string_dict(
            doc = "Environment variables for the action.",
        ),
        "exit_code": attr.output(mandatory = True),
        "log": attr.output(
            doc = "Log file; its directory is the results directory.",
            mandatory = True,
        ),
        "manifest": attr.output(
            doc = "(result, golden, ignore) triples recorded by diff_files.",
            mandatory = True,
        ),
        "openroad": attr.label(
            executable = True,
            # Target, not exec: avoids a second build of openroad in the
            # exec configuration. Fine as long as host == target.
            cfg = "target",
            mandatory = True,
        ),
        "results": attr.output_list(
            doc = "Side outputs written via make_result_file, in the " +
                  "same directory as the log. A name ending in .sha256 " +
                  "holds the digest of the file without that suffix.",
        ),
        "test_file": attr.label(allow_single_file = [".tcl"], mandatory = True),
        "timeout": attr.int(
            doc = "Seconds before openroad is killed (exit code 124).",
            mandatory = True,
        ),
        "_run_sh": attr.label(
            default = "//test:regression_run.sh",
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
    # Hermetic timeout and sha256sum, so the host needs neither.
    toolchains = ["@bazel_lib//lib:coreutils_toolchain_type"],
)

def _golden_test_impl(ctx):
    if len(ctx.files.goldens) != len(ctx.files.results):
        fail("goldens and results must be parallel lists")
    pairs = " ".join([
        "{}:{}".format(g.short_path, r.short_path)
        for g, r in zip(ctx.files.goldens, ctx.files.results)
    ])
    script = ctx.actions.declare_file(ctx.label.name + ".sh")
    ctx.actions.expand_template(
        template = ctx.file._golden_test_sh,
        output = script,
        substitutions = {
            "@DEBUG_DIR@": ctx.attr.debug_dir,
            "@DEBUG_TARGET@": ctx.attr.debug_target,
            "@EXIT_CODE@": ctx.file.exit_code.short_path,
            "@EXPECTED_EXIT_CODE@": str(ctx.attr.expected_exit_code),
            "@GOLDEN_LOG@": ctx.file.golden_log.short_path,
            "@LOG@": ctx.file.log.short_path,
            "@MANIFEST@": ctx.file.manifest.short_path,
            "@PAIRS@": pairs,
            "@RECORDED@": " ".join(ctx.attr.recorded),
            "@RUN_TIMEOUT@": str(ctx.attr.run_timeout),
            "@UPDATE_TARGET@": ctx.attr.update_target,
        },
        is_executable = True,
    )
    return [DefaultInfo(
        executable = script,
        runfiles = ctx.runfiles(
            files = [
                ctx.file.exit_code,
                ctx.file.golden_log,
                ctx.file.log,
                ctx.file.manifest,
            ] + ctx.files.goldens + ctx.files.results,
        ),
    )]

golden_test = rule(
    implementation = _golden_test_impl,
    doc = "Compare regression_run outputs against checked-in goldens.",
    attrs = {
        "debug_dir": attr.string(),
        "debug_target": attr.string(),
        "exit_code": attr.label(allow_single_file = True, mandatory = True),
        "expected_exit_code": attr.int(default = 0),
        "golden_log": attr.label(allow_single_file = True, mandatory = True),
        "goldens": attr.label_list(allow_files = True),
        "log": attr.label(allow_single_file = True, mandatory = True),
        "manifest": attr.label(allow_single_file = True, mandatory = True),
        "recorded": attr.string_list(
            doc = "result:golden basenames expected from diff_files calls.",
        ),
        "results": attr.label_list(allow_files = True),
        "run_timeout": attr.int(mandatory = True),
        "update_target": attr.string(mandatory = True),
        "_golden_test_sh": attr.label(
            default = "//test:golden_test.sh.tpl",
            allow_single_file = True,
        ),
    },
    test = True,
)

def golden_regression_test(
        name,
        goldens = {},
        data = [],
        env = None,
        expected_exit_code = 0,
        hash_goldens = False,
        run_timeout = 900,
        size = "small",
        tags = []):
    """Tcl regression whose goldens can be updated without re-running it.

    Args:
        name: Test stem; <name>.tcl is run and <name>.ok is the golden log.
        goldens: Side-output goldens, {golden file: result file name}, where
            the result name is what make_result_file produces (e.g.
            {"foo.defok": "foo-tcl.def"}).
        data: Every file the run reads, listed explicitly. The sandbox
            hides anything else, so a missing input fails the run. Must not
            include goldens, or updating one invalidates the cached run.
        env: Environment for the run (defaults to GPU_ENV_OFF).
        expected_exit_code: Expected openroad exit code.
        hash_goldens: Check in <golden>.sha256 instead of each side-output
            golden.
        run_timeout: Seconds before the openroad run is killed. Build
            actions have no Bazel timeout, and remote execution caps
            non-test actions at one hour by default, so keep this below that.
        size: Test size. The test itself is cheap; this only matters for
            the timeout, since the run happens at build time.
        tags: Tags for the test.
    """
    if env == None:
        env = GPU_ENV_OFF
    run = name + "-tcl_run"
    results_dir = name + "-tcl_results/"
    log = results_dir + name + "-tcl.log"
    update = name + "_update"
    suffix = ".sha256" if hash_goldens else ""

    # Checked-in golden -> output it is compared against.
    checked = {name + ".ok": log}
    for golden, result in goldens.items():
        checked[golden + suffix] = results_dir + result + suffix

    # Run, debug and update targets are manual so `bazel build //...` does not
    # execute every regression, and testonly so only tests can depend on
    # them; `bazel test` still builds them as deps.
    regression_run(
        name = run,
        test_file = name + ".tcl",
        data = data,
        env = env,
        log = log,
        exit_code = results_dir + "exit_code",
        manifest = results_dir + "goldens.manifest",
        results = list(checked.values())[1:],
        openroad = "//:openroad",
        tags = ["manual"],
        testonly = True,
        timeout = run_timeout,
    )

    debug = None
    debug_dir = name + "-tcl_debug/"
    if hash_goldens:
        # Same run, keeping the full side outputs. Not used by the test; build
        # it by hand to see what changed behind a digest mismatch.
        debug = name + "-tcl_debug"
        regression_run(
            name = debug,
            test_file = name + ".tcl",
            data = data,
            env = env,
            log = debug_dir + name + "-tcl.log",
            exit_code = debug_dir + "exit_code",
            manifest = debug_dir + "goldens.manifest",
            results = [debug_dir + r for r in goldens.values()],
            openroad = "//:openroad",
            tags = ["manual"],
            testonly = True,
            timeout = run_timeout,
        )

    golden_test(
        name = name + "-tcl_test",
        log = log,
        exit_code = results_dir + "exit_code",
        manifest = results_dir + "goldens.manifest",
        expected_exit_code = expected_exit_code,
        run_timeout = run_timeout,
        golden_log = name + ".ok",
        goldens = list(checked.keys())[1:],
        results = list(checked.values())[1:],
        recorded = [r + ":" + g for g, r in goldens.items()],
        debug_dir = "bazel-bin/{}/{}".format(native.package_name(), debug_dir),
        debug_target = "//{}:{}".format(native.package_name(), debug) if debug else "",
        update_target = "//{}:{}".format(native.package_name(), update),
        size = size,
        # .bazelrc excludes this tag from `bazel build`; building the test
        # would otherwise run openroad, since the run is in its runfiles.
        tags = tags + ["golden_regression", "tcl"],
    )

    write_source_files(
        name = update,
        files = checked,
        diff_test = False,
        tags = ["manual"],
        testonly = True,
    )
