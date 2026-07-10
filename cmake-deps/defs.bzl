# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Materialize OpenROAD's third-party dependencies as a CMake prefix.

`bazelisk run //:cmake` copies the bundle built here into `<workspace>/deps`
so OpenROAD can be built with plain CMake against bazel-built dependencies
and the hermetic clang toolchain — no DependencyInstaller.sh, no sudo, no
host compilers. See docs/user/Build.md.

The bundle is assembled by a single cached action:
  include/     merged header tree of every dependency CMake looks up
  lib/pool/    every static archive at its repo-relative path
  lib/         conventionally named copies for find_library-based modules
  lib/cmake/   package config shims + generated pool/versions include files
  lib/tcl9.0/  the Tcl script library (init.tcl etc.)
  llvm/        the hermetic clang toolchain, staged exactly as bazel does
  bin/         cc/c++/swig/bison/flex wrapper scripts
  tools/       swig/bison/flex binaries and support trees
  python/      hermetic CPython (interpreter, headers, libpython, stdlib)

Compiler and linker flags are extracted from the resolved C++ toolchain at
analysis time (the rules_foreign_cc technique), so they cannot drift from
what bazel itself uses when the `llvm` module is bumped.
"""

load("@rules_cc//cc:action_names.bzl", "ACTION_NAMES")
load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cc_toolchain", "use_cc_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

# Flags owned by the CMake build type, not by the toolchain. Everything else
# extracted from the bazel toolchain is kept verbatim.
#
# -D_FORTIFY_SOURCE is stripped here but re-injected by the wrapper with a
# value keyed to the optimization level: in boost 1.89 defined(_FORTIFY_SOURCE)
# selects a 128-byte asio ip::detail::endpoint layout the prebuilt archives
# are compiled with, so the define must be present in every build type or the
# archive's endpoint ctor overflows the caller's smaller stack object (ABI
# break). Value 0 under -O0 keeps defined() true while satisfying glibc's
# "fortify requires optimization" check.
_FLAG_DENYLIST_PREFIXES = [
    "-O",
    "-g",
    "-DNDEBUG",
    "-D_FORTIFY_SOURCE",
    "-std=",
    "-frandom-seed",
    "-ffile-compilation-dir",
    "-fdebug-prefix-map",
]

_FLAG_DENYLIST_EXACT = [
    "-c",
    "-S",
    "-E",
]

def _keep_flag(flag):
    if flag in _FLAG_DENYLIST_EXACT:
        return False
    for prefix in _FLAG_DENYLIST_PREFIXES:
        if flag.startswith(prefix):
            return False

    # Warning flags are per-project policy; -Wl,/-Wa,/-Wp, are not
    # warnings, and -Wno- flags only silence noise the kept toolchain
    # flags themselves cause (e.g. the __DATE__ redaction).
    if flag.startswith("-W") and not (
        flag.startswith("-Wl,") or
        flag.startswith("-Wa,") or
        flag.startswith("-Wp,") or
        flag.startswith("-Wno-")
    ):
        return False
    return True

def _map_exec_path(path):
    """Map an exec path to its stable location inside the bundle.

    Source files keep their exec path; generated files drop the
    configuration segment so the layout is configuration-independent:
      external/X               -> external/X
      bazel-out/<cfg>/bin/X    -> gen/X
    """
    if path.startswith("bazel-out/"):
        parts = path.split("/")
        if len(parts) > 3 and parts[2] == "bin":
            return "gen/" + "/".join(parts[3:])
        return "gen/" + "/".join(parts[2:])
    return path

def _rewrite_flag(flag, subtree):
    """Rewrite exec paths embedded in a toolchain flag to bundle paths.

    "$R" is a placeholder the wrapper script resolves to the prefix root.
    """
    for marker in ["bazel-out/", "external/"]:
        prefix, found, rest = flag.partition(marker)

        # Only rewrite when the marker starts a path: at the start of the
        # flag, after '=' (--sysroot=external/...), or after a short
        # option (-Lbazel-out/..., -Bbazel-out/...).
        if found and (
            prefix == "" or
            prefix.endswith("=") or
            (prefix.startswith("-") and "/" not in prefix)
        ):
            return prefix + "$R/" + subtree + "/" + _map_exec_path(marker + rest)
    return flag

def _shell_quote(arg):
    """Double-quote for bash, keeping ${R} expandable.

    Toolchain flags contain literal quotes (-D__DATE__="redacted") that
    must survive into the macro value. Any other literal $ is escaped so
    bash does not expand it; only the ${R} placeholder stays live.
    """
    escaped = (
        arg
            .replace("\\", "\\\\")
            .replace('"', '\\"')
            .replace("`", "\\`")
            .replace("$", "\\$")
            .replace("\\${R}", "${R}")
    )
    return '"' + escaped + '"'

def _module_name(workspace_name):
    """Bazel module name of a canonical repository name.

    "googletest+" and "rules_python++python+python_3_13_x86_64-..." both
    map to their leading module name.
    """
    return workspace_name.partition("+")[0]

def _toolchain_command_lines(ctx, cc_toolchain):
    """Extract compile and link command lines from the resolved toolchain."""
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    compile_variables = cc_common.create_compile_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        user_compile_flags = [],
    )
    link_variables = cc_common.create_link_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        is_static_linking_mode = True,
        is_linking_dynamic_library = False,
        user_link_flags = [],
    )

    def flags(action_name, variables):
        return [
            f
            for f in cc_common.get_memory_inefficient_command_line(
                feature_configuration = feature_configuration,
                action_name = action_name,
                variables = variables,
            )
            if _keep_flag(f)
        ]

    def tool(action_name):
        return cc_common.get_tool_for_action(
            feature_configuration = feature_configuration,
            action_name = action_name,
        )

    return struct(
        feature_configuration = feature_configuration,
        c_compiler = tool(ACTION_NAMES.c_compile),
        cxx_compiler = tool(ACTION_NAMES.cpp_compile),
        archiver = tool(ACTION_NAMES.cpp_link_static_library),
        c_flags = flags(ACTION_NAMES.c_compile, compile_variables),
        cxx_flags = flags(ACTION_NAMES.cpp_compile, compile_variables),
        link_flags = flags(ACTION_NAMES.cpp_link_executable, link_variables),
    )

def _repo_relative_path(file):
    """Path of `file` inside its repository, without the repo prefix."""
    marker = "external/" + file.owner.workspace_name + "/"
    _, _, rest = file.path.partition(marker)
    return rest or file.path

def _include_roots(compilation_context):
    roots = []
    for root in (
        compilation_context.includes.to_list() +
        compilation_context.system_includes.to_list() +
        compilation_context.quote_includes.to_list() +
        compilation_context.external_includes.to_list()
    ):
        # "." (the exec root) and bare bazel-out bin dirs would claim every
        # header with a bogus destination; real include dirs win by length.
        if root == "." or (root.startswith("bazel-out/") and root.endswith("/bin")):
            continue
        roots.append(root)
    return roots

def _header_dest(header, sorted_roots, include_overrides):
    """Destination of a header inside include/, or None to skip it."""
    path = header.path
    for root in sorted_roots:
        if path.startswith(root + "/"):
            rel = path[len(root) + 1:]
            override = include_overrides.get(_module_name(header.owner.workspace_name))
            if override:
                return "include/" + override + "/" + rel
            return "include/" + rel
    return None

def _archive_dest(library_file):
    """Destination of a static archive inside lib/pool/."""
    _, _, rest = library_file.path.partition("external/")
    return "lib/pool/" + (rest or library_file.path)

def _wrapper_script(compiler, flags, link_args, defines):
    """A relocatable compiler driver wrapper.

    The toolchain flags always apply. Link inputs (the static C++ runtime
    archives and the toolchain link flags) must come after user objects and
    libraries, and only when the driver actually links.
    """
    lines = [
        "#!/usr/bin/env bash",
        "# Generated by //cmake-deps. Wraps the hermetic clang toolchain",
        "# with the exact flags the bazel build uses.",
        "set -u",
        'R="$(cd "$(dirname "$0")/.." && pwd)"',
        "link=1",
        'fortify="-D_FORTIFY_SOURCE=0"',
        'for arg in "$@"; do',
        '  case "$arg" in',
        "    -c|-S|-E|-M|-MM|--version|-v|-dumpversion|-dumpmachine|--help|-print-*)",
        "      link=0",
        "      ;;",
        "    -O|-O1|-O2|-O3|-Os|-Oz|-Ofast|-Og)",
        '      fortify="-D_FORTIFY_SOURCE=1"',
        "      ;;",
        "  esac",
        "done",
        "post=()",
        'if [[ "$link" -eq 1 && "$#" -gt 0 ]]; then',
        "  post=(" + " ".join(
            ["-Qunused-arguments"] + [
                _shell_quote(arg.replace("$R", "${R}"))
                for arg in link_args
            ],
        ) + ")",
        "fi",
    ]
    args = [_shell_quote(f.replace("$R", "${R}")) for f in flags]
    args += [_shell_quote("-D" + d) for d in defines]
    lines.append(
        'exec "${R}/llvm/' + _map_exec_path(compiler) + '" \\\n  ' +
        " \\\n  ".join(args) +
        ' \\\n  "${fortify}" "$@" ${post[@]+"${post[@]}"}',
    )
    return "\n".join(lines) + "\n"

def _tool_wrapper_script(tool_path, env):
    """A relocatable wrapper for a build tool (swig/bison/flex)."""
    lines = [
        "#!/usr/bin/env bash",
        "# Generated by //cmake-deps.",
        "set -u",
        'R="$(cd "$(dirname "$0")/.." && pwd)"',
    ]
    for key, value in sorted(env.items()):
        lines.append('export {}="{}"'.format(key, value.replace("$R", "${R}")))
    lines.append('exec "' + tool_path.replace("$R", "${R}") + '" "$@"')
    return "\n".join(lines) + "\n"

def _pool_cmake(pool_dests, alwayslink_dests):
    """lib/cmake/openroad_deps/deps-pool.cmake content.

    One interface target carrying every archive in a link group; static
    over-linking is free (the linker only extracts needed members) and
    makes archive order irrelevant.
    """
    lines = [
        "# Generated by //cmake-deps.",
        'get_filename_component(_OR_DEPS "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)',
        "if(TARGET openroad_deps::pool)",
        "  return()",
        "endif()",
        "add_library(openroad_deps::pool INTERFACE IMPORTED GLOBAL)",
        "set(_or_deps_pool",
        "  -Wl,--start-group",
        "  -Wl,--whole-archive",
    ]
    for dest in alwayslink_dests:
        lines.append('  "${_OR_DEPS}/' + dest + '"')
    lines.append("  -Wl,--no-whole-archive")
    for dest in pool_dests:
        lines.append('  "${_OR_DEPS}/' + dest + '"')
    lines += [
        "  -Wl,--end-group",
        ")",
        "set_target_properties(openroad_deps::pool PROPERTIES",
        '  INTERFACE_LINK_LIBRARIES "${_or_deps_pool}"',
        ")",
    ]
    return "\n".join(lines) + "\n"

def _shim_package(basename):
    for suffix in ["ConfigVersion.cmake", "Config.cmake"]:
        if basename.endswith(suffix):
            return basename[:-len(suffix)]
    fail("shim file name must end in Config.cmake or ConfigVersion.cmake: " + basename)

def _cmake_deps_bundle_impl(ctx):
    cc_toolchain = find_cc_toolchain(ctx)
    toolchain = _toolchain_command_lines(ctx, cc_toolchain)
    static_runtimes = cc_toolchain.static_runtime_lib(
        feature_configuration = toolchain.feature_configuration,
    )

    copies = {}  # dest -> src exec path
    inputs = [cc_toolchain.all_files, static_runtimes]

    # --- Hermetic clang toolchain, staged exactly as bazel stages it. ---
    for f in cc_toolchain.all_files.to_list() + static_runtimes.to_list():
        copies["llvm/" + _map_exec_path(f.path)] = f.path

    # lib/ carries the conventionally named archives (libomp.a etc.), so
    # driver-added libraries like -fopenmp's -lomp resolve there.
    runtime_link_args = ["-L$R/lib"] + [
        "$R/llvm/" + _map_exec_path(f.path)
        for f in static_runtimes.to_list()
    ]

    # --- Headers and archives of every dependency. ---
    roots = {}
    for dep in ctx.attr.deps:
        for root in _include_roots(dep[CcInfo].compilation_context):
            roots[root] = None
    sorted_roots = sorted(roots.keys(), key = len, reverse = True)

    # Propagated defines are collected across all deps and injected
    # globally by the compiler wrappers. This mirrors bazel, where these
    # defines reach every target that (transitively) depends on the
    # dependency — for openroad, all of them. Per-package attribution via
    # INTERFACE_COMPILE_DEFINITIONS on the shims cannot cover the
    # dependencies CMake consumes without an imported target (Tcl and
    # zlib are plain cache-variable paths), and a header compiled with a
    # different define set than its archive (e.g. _FILE_OFFSET_BITS=64)
    # is an ABI break.
    all_defines = {}
    pool = {}  # ordered set: dest -> None
    alwayslink_pool = {}
    module_archives = {}  # module name -> [dest, ...]

    for dep in ctx.attr.deps:
        cc_info = dep[CcInfo]
        inputs.append(cc_info.compilation_context.headers)
        for define in cc_info.compilation_context.defines.to_list():
            all_defines[define] = None
        for header in cc_info.compilation_context.headers.to_list():
            dest = _header_dest(header, sorted_roots, ctx.attr.include_overrides)
            if dest == None:
                # Private header without a matching include root: not part
                # of any dependency's public interface.
                continue
            if copies.get(dest, header.path) != header.path:
                # Same header staged via several include roots (e.g.
                # protobuf _virtual_includes); the assembler verifies the
                # contents are identical.
                continue
            copies[dest] = header.path

        for linker_input in cc_info.linking_context.linker_inputs.to_list():
            for library in linker_input.libraries:
                archive = library.pic_static_library or library.static_library
                if archive == None:
                    if library.pic_objects or library.objects:
                        fail("{}: archive-less object list; not supported".format(
                            linker_input.owner,
                        ))
                    continue
                inputs.append(depset([archive]))
                dest = _archive_dest(archive)
                copies[dest] = archive.path
                module = _module_name(linker_input.owner.workspace_name)
                module_archives.setdefault(module, [])
                if dest not in module_archives[module]:
                    module_archives[module].append(dest)
                if module in ctx.attr.exclude_from_pool:
                    continue
                if library.alwayslink:
                    alwayslink_pool[dest] = None
                else:
                    pool[dest] = None

    # Conventionally named copies for find_library-based CMake modules.
    for module, name in ctx.attr.lib_name_overrides.items():
        archives = module_archives.get(module)
        if not archives:
            fail("lib_name_overrides: no archive found for module " + module)
        if len(archives) > 1:
            fail("lib_name_overrides: module {} has {} archives".format(
                module,
                len(archives),
            ))
        copies["lib/lib{}.a".format(name)] = copies[archives[0]]

    # Modules whose archives the static shims reference by path: copy them
    # to lib/<basename> so the shims need no bzlmod canonical repository
    # names (lib/pool/ paths embed e.g. "googletest+", which is not stable
    # across bazel versions).
    for module in ctx.attr.stable_lib_modules:
        for dest in module_archives.get(module, []):
            basename = dest.split("/")[-1]
            if copies.get("lib/" + basename, copies[dest]) != copies[dest]:
                fail("stable_lib_modules: lib/{} already taken".format(basename))
            copies["lib/" + basename] = copies[dest]

    def single_archive(module):
        archives = module_archives.get(module)
        if not archives or len(archives) != 1:
            fail("expected exactly one archive for module " + module)
        return archives[0]

    # --- Tcl script library (init.tcl etc.), needed at openroad runtime. ---
    for f in ctx.files.tcl_library:
        rel = _repo_relative_path(f)
        _, _, in_library = rel.partition("library/")
        copies["lib/tcl9.0/" + (in_library or rel)] = f.path
    inputs.append(depset(ctx.files.tcl_library))

    # --- SWIG: binary and runtime library tree. ---
    swig_binary = ctx.executable.swig
    copies["tools/swig/swig"] = swig_binary.path
    for f in ctx.files.swig_lib:
        rel = _repo_relative_path(f)
        _, _, in_lib = rel.partition("Lib/")
        copies["share/swig/" + (in_lib or rel)] = f.path
    inputs.append(depset(ctx.files.swig_lib + [swig_binary]))

    # --- bison / flex from their toolchains. ---
    # Their env values point into the tool's runfiles tree; rewrite them to
    # the bundle the same way bazel/bison.bzl rewrites them for actions:
    # data files live at their source paths, binaries under bin.
    bison = ctx.toolchains["@rules_bison//bison:toolchain_type"].bison_toolchain
    flex = ctx.toolchains["@rules_flex//flex:toolchain_type"].flex_toolchain
    tool_wrappers = {}
    for name, info in [("bison", bison), ("flex", flex)]:
        tool = getattr(info, name + "_tool").executable
        env = dict(getattr(info, name + "_env"))
        inputs.append(info.all_files)
        for f in info.all_files.to_list():
            copies["tools/" + _map_exec_path(f.path)] = f.path
        runfiles_dir = "{}.runfiles/{}".format(tool.path, tool.owner.workspace_name)
        source_form = "$R/tools/external/" + tool.owner.workspace_name  # buildifier: disable=external-path
        binary_form = "$R/tools/" + _map_exec_path(
            "{}/external/{}".format(tool.root.path, tool.owner.workspace_name),  # buildifier: disable=external-path
        )
        for key, value in env.items():
            form = source_form if key == "BISON_PKGDATADIR" else binary_form
            env[key] = value.replace(runfiles_dir, form)
        tool_wrappers[name] = _tool_wrapper_script(
            "$R/tools/" + _map_exec_path(tool.path),
            env,
        )

    # --- Hermetic CPython: interpreter, stdlib, headers, libpython. ---
    py_runtime = ctx.toolchains["@rules_python//python:toolchain_type"].py3_runtime
    python_dynamic_libs = [
        library.dynamic_library
        for linker_input in ctx.attr.python_libs[CcInfo].linking_context.linker_inputs.to_list()
        for library in linker_input.libraries
        if library.dynamic_library
    ]
    python_files = depset(
        transitive = [
            py_runtime.files,
            ctx.attr.python_headers[CcInfo].compilation_context.headers,
        ],
    )
    for f in python_files.to_list():
        rel = _repo_relative_path(f)
        if rel == f.path:
            # Not inside the python repository (e.g. bazel's _solib copy
            # of libpython, staged by basename below).
            continue
        copies["python/" + rel] = f.path

    # FindPython3's Development component wants the unversioned dev name
    # (libpython3.13.so); the cc_libs artifact carries exactly that.
    for f in python_dynamic_libs:
        copies["python/lib/" + f.basename] = f.path
    inputs.append(depset(python_dynamic_libs, transitive = [python_files]))
    version_info = py_runtime.interpreter_version_info
    python_version = "{}.{}".format(version_info.major, version_info.minor)

    # --- CMake package config shims. ---
    for f in ctx.files.shims:
        copies["lib/cmake/{}/{}".format(_shim_package(f.basename), f.basename)] = f.path
    inputs.append(depset(ctx.files.shims))

    # --- Generated text files. ---
    defines = sorted(all_defines.keys())
    writes = [
        struct(
            dest = "lib/cmake/openroad_deps/deps-pool.cmake",
            content = _pool_cmake(pool.keys(), alwayslink_pool.keys()),
            executable = False,
        ),
        struct(
            dest = "bin/cc",
            content = _wrapper_script(
                toolchain.c_compiler,
                [_rewrite_flag(f, "llvm") for f in toolchain.c_flags],
                [_rewrite_flag(f, "llvm") for f in toolchain.link_flags] +
                runtime_link_args,
                defines,
            ),
            executable = True,
        ),
        struct(
            dest = "bin/c++",
            content = _wrapper_script(
                toolchain.cxx_compiler,
                [_rewrite_flag(f, "llvm") for f in toolchain.cxx_flags],
                [_rewrite_flag(f, "llvm") for f in toolchain.link_flags] +
                runtime_link_args,
                defines,
            ),
            executable = True,
        ),
        struct(
            dest = "bin/swig",
            content = _tool_wrapper_script(
                "$R/tools/swig/swig",
                {"SWIG_LIB": "$R/share/swig"},
            ),
            executable = True,
        ),
        struct(
            dest = "bin/bison",
            content = tool_wrappers["bison"],
            executable = True,
        ),
        struct(
            dest = "bin/flex",
            content = tool_wrappers["flex"],
            executable = True,
        ),
    ]

    manifest = struct(
        copies = [struct(dest = dest, src = src) for dest, src in copies.items()],
        writes = writes,
        module_bazel = ctx.file.versions_src.path,
        versions_dest = "lib/cmake/openroad_deps/deps-versions.cmake",
        template_src = ctx.file.toolchain_template.path,
        template_dest = "toolchain.cmake",
        substitutions = {
            "@ARCHIVER@": "${_OR_DEPS}/llvm/" + _map_exec_path(toolchain.archiver),
            "@OMP_ARCHIVE@": "${_OR_DEPS}/" + single_archive("openmp"),
            "@PYTHON_VERSION@": python_version,
            "@TCL_ARCHIVE@": "${_OR_DEPS}/" + single_archive("tcl_lang"),
        },
    )
    inputs.append(depset([ctx.file.versions_src, ctx.file.toolchain_template]))

    manifest_file = ctx.actions.declare_file(ctx.label.name + "-manifest.json")
    ctx.actions.write(manifest_file, json.encode_indent(manifest))

    bundle = ctx.actions.declare_directory(ctx.label.name)
    ctx.actions.run(
        outputs = [bundle],
        inputs = depset([manifest_file], transitive = inputs),
        executable = ctx.executable._assembler,
        arguments = [manifest_file.path, bundle.path],
        mnemonic = "CMakeDepsBundle",
        progress_message = "Assembling CMake dependency prefix %{output}",
    )

    return [DefaultInfo(files = depset([bundle]))]

cmake_deps_bundle = rule(
    implementation = _cmake_deps_bundle_impl,
    doc = "Assemble a CMake dependency prefix from bazel-built dependencies.",
    attrs = {
        "deps": attr.label_list(
            providers = [CcInfo],
            doc = "Dependencies whose headers and archives go into the prefix.",
        ),
        "exclude_from_pool": attr.string_list(
            doc = "Module names whose archives are staged but kept out of " +
                  "the link pool (e.g. googletest, linked per-test).",
        ),
        "include_overrides": attr.string_dict(
            doc = "Module name -> include/ subdirectory. Diverts a " +
                  "repository's headers to avoid basename collisions " +
                  "(e.g. cudd's util.h).",
        ),
        "lib_name_overrides": attr.string_dict(
            doc = "Module name -> conventional archive name. Creates " +
                  "lib/lib<name>.a copies for CMake Find modules that " +
                  "search by name (FindZLIB: z, FindCUDD: cudd).",
        ),
        "python_headers": attr.label(
            providers = [CcInfo],
            mandatory = True,
            doc = "CPython headers (rules_python current_py_cc_headers).",
        ),
        "python_libs": attr.label(
            providers = [CcInfo],
            mandatory = True,
            doc = "libpython (rules_python current_py_cc_libs).",
        ),
        "shims": attr.label_list(
            allow_files = [".cmake"],
            doc = "CMake package config shims, staged by basename into " +
                  "lib/cmake/<Package>/.",
        ),
        "stable_lib_modules": attr.string_list(
            doc = "Module names whose archives are also copied to " +
                  "lib/<basename>, giving the static config shims paths " +
                  "free of bzlmod canonical repository names.",
        ),
        "swig": attr.label(
            executable = True,
            cfg = "exec",
            mandatory = True,
            doc = "The swig binary.",
        ),
        "swig_lib": attr.label_list(
            allow_files = True,
            doc = "SWIG runtime library (Lib/ tree).",
        ),
        "tcl_library": attr.label(
            allow_files = True,
            mandatory = True,
            doc = "The Tcl script library (@tcl_lang//:tcl_core).",
        ),
        "toolchain_template": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "toolchain.cmake template, staged at the prefix root.",
        ),
        "versions_src": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "MODULE.bazel, parsed for dependency versions.",
        ),
        "_assembler": attr.label(
            default = ":assemble_bundle",
            executable = True,
            cfg = "exec",
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cc_toolchain() + [
        "@rules_bison//bison:toolchain_type",
        "@rules_flex//flex:toolchain_type",
        "@rules_python//python:toolchain_type",
    ],
)
