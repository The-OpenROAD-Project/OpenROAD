"""Module extension to fetch and build yosys-slang plugin."""

load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")

def _yosys_slang_impl(_module_ctx):
    new_git_repository(
        name = "yosys-slang",
        remote = "https://github.com/povik/yosys-slang.git",
        commit = "4e53d772996184b07e9bfe784060f96e6cb0a267",
        init_submodules = True,
        build_file = Label("//bazel:yosys_slang.BUILD.bazel"),
    )

yosys_slang = module_extension(
    implementation = _yosys_slang_impl,
)
