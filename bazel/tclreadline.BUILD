# BUILD file for tclreadline
load("@rules_cc//cc:defs.bzl", "cc_library")

# tclreadline needs headers from TCL and Readline.
# We assume the host system has TCL and Readline development headers installed,
# as OpenROAD already relies on them.

# Generate tclreadline.h from tclreadline.h.in
genrule(
    name = "generate_tclreadline_h",
    srcs = ["tclreadline.h.in"],
    outs = ["tclreadline.h"],
    cmd = "sed " +
          "-e 's|@TCLRL_DIR@|/usr/lib/tclreadline|g' " +
          "-e 's|@VERSION@|2.3.8|g' " +
          "-e 's|@PATCHLEVEL_STR@|2.3.8|g' " +
          "-e 's|@MAJOR@|2|g' " +
          "-e 's|@MINOR@|3|g' " +
          "-e 's|@PATCHLEVEL@|8|g' " +
          "$(location tclreadline.h.in) > $@",
)

# Generate config.h
genrule(
    name = "generate_config_h",
    srcs = ["config.h.in"],
    outs = ["config.h"],
    cmd = "sed " +
          "-e 's|#undef HAVE_TCL_H|#define HAVE_TCL_H 1|g' " +
          "-e 's|#undef HAVE_READLINE_READLINE_H|#define HAVE_READLINE_READLINE_H 1|g' " +
          "-e 's|#undef HAVE_READLINE_HISTORY_H|#define HAVE_READLINE_HISTORY_H 1|g' " +
          "-e 's|#undef EXECUTING_MACRO_HACK|#define EXECUTING_MACRO_HACK 1|g' " +
          "-e 's|#undef HAVE_RL_COMPLETION_MATCHES|#define HAVE_RL_COMPLETION_MATCHES 1|g' " +
          "-e 's|#undef HAVE_RL_DING|#define HAVE_RL_DING 1|g' " +
          "$(location config.h.in) > $@",
)

# Generate tclreadlineInit.tcl from tclreadlineInit.tcl.in
genrule(
    name = "generate_tclreadlineInit_tcl",
    srcs = ["tclreadlineInit.tcl.in"],
    outs = ["tclreadlineInit.tcl"],
    cmd = "sed " +
          "-e 's|@VERSION@|2.3.8|g' " +
          "-e 's|@TCLRL_LIBDIR@|/usr/lib/tclreadline|g' " +
          "-e 's|\\[file join \\[file dirname \\[info script\\]\\] tclreadlineSetup.tcl\\]|$$::tclreadline::setup_path|g' " +
          "-e 's|\\[file join \\[file dirname \\[info script\\]\\] tclreadlineCompleter.tcl\\]|$$::tclreadline::completer_path|g' " +
          "$(location tclreadlineInit.tcl.in) > $@",
)

# Generate tclreadlineSetup.tcl from tclreadlineSetup.tcl.in
genrule(
    name = "generate_tclreadlineSetup_tcl",
    srcs = ["tclreadlineSetup.tcl.in"],
    outs = ["tclreadlineSetup.tcl"],
    cmd = "sed " +
          "-e 's|@TCLRL_DIR@|$$::tclreadline::library|g' " +
          "-e 's|@VERSION@|2.3.8|g' " +
          "$(location tclreadlineSetup.tcl.in) > $@",
)

genrule(
    name = "patch_tclreadline_c",
    srcs = ["tclreadline.c"],
    outs = ["tclreadline_patched.c"],
    cmd = "sed -e 's@\"::tclreadline::library\"@\"::tclreadline::library_dummy_ignored\"@g' " +
          "-e 's@\"tclreadline_library\"@\"tclreadline_library_dummy_ignored\"@g' $< > $@",
)

# Generate tclIndex using a simple tcl script since we don't assume the target
# system has a full tclsh available at cross-compile time (but openroad does).
# Actually, the simplest way is to manually create the tclIndex for the files.
genrule(
    name = "generate_tclIndex",
    srcs = [
        "tclreadlineInit.tcl",
        "tclreadlineSetup.tcl",
        "tclreadlineCompleter.tcl",
    ],
    outs = ["tclIndex"],
    cmd = "echo 'set auto_index(::tclreadline::Completer) [list source [file join $$dir tclreadlineCompleter.tcl]]' > $@ && " +
          "echo 'set auto_index(::tclreadline::Setup) [list source [file join $$dir tclreadlineSetup.tcl]]' >> $@ && " +
          "echo 'set auto_index(::tclreadline::Init) [list source [file join $$dir tclreadlineInit.tcl]]' >> $@",
)

cc_library(
    name = "tclreadline",
    srcs = [
        "config.h",
        "tclreadline_patched.c",
    ],
    hdrs = [
        "tclreadline.h",
    ],
    copts = [
        "-Wno-implicit-function-declaration",
        "-Wno-int-conversion",
        "-Wno-deprecated-declarations",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@readline",
        "@tcl_lang//:tcl",
    ],
)

# Exposes the path to tclreadlineInit.tcl as a cc_library define,
# so Main.cc can load the correct init script at runtime.
# We embed the path that we expect the runfiles or an absolute
# path where the user might choose to install it. Since it's bazel,
# we need to ensure the init script is available. For now we just embed ENABLE_READLINE.
cc_library(
    name = "tclreadline_defs",
    defines = [
        "ENABLE_READLINE",
    ],
    visibility = ["//visibility:public"],
)

# We package the tcl scripts that tclreadline requires to run correctly at runtime.
filegroup(
    name = "tclreadline_scripts",
    srcs = [
        "tclIndex",
        "tclreadlineCompleter.tcl",
        "tclreadlineInit.tcl",
        "tclreadlineSetup.tcl",
    ],
    visibility = ["//visibility:public"],
)
