workspace(name = "openroad")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

http_archive(
    name = "rules_cc",
    sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
    strip_prefix = "rules_cc-0.0.9",
    urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz"],
)

http_archive(
    name = "com_grail_bazel_toolchain",
    sha256 = "ddad1bde0eb9d470ea58500681a7deacdf55c714adf4b89271392c4687acb425",
    strip_prefix = "toolchains_llvm-7e7c7cf1f965f348861085183d79b6a241764390",
    urls = ["https://github.com/grailbio/bazel-toolchain/archive/7e7c7cf1f965f348861085183d79b6a241764390.tar.gz"],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

maybe(
    http_archive,
    name = "rules_python",
    sha256 = "e3f1cc7a04d9b09635afb3130731ed82b5f58eadc8233d4efb59944d92ffc06f",
    strip_prefix = "rules_python-0.33.2",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.33.2/rules_python-0.33.2.tar.gz",
)

load(
    "@rules_python//python:repositories.bzl",
    "py_repositories",
    "python_register_toolchains",
)

# Must be called before using anything from rules_python.
# https://github.com/bazelbuild/rules_python/issues/1560#issuecomment-1815118394
py_repositories()

python_register_toolchains(
    name = "python39",

    # Required for our containerized CI environments; we do not recommend
    # building bazel_rules_hdl as root normally.
    ignore_root_user_error = True,
    python_version = "3.9",
)

load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

llvm_toolchain(
    name = "llvm_toolchain",
    llvm_version = "10.0.1",
    sha256 = {
        "linux": "02a73cfa031dfe073ba8d6c608baf795aa2ddc78eed1b3e08f3739b803545046",
    },
    strip_prefix = {
        "linux": "clang+llvm-10.0.1-x86_64-pc-linux-gnu",
    },
    urls = {
        "linux": [
            # Use a custom built Clang+LLVM binrary distribution that is more portable than
            # the official builds because it's built against an older glibc and does not have
            # dynamic library dependencies to tinfo, gcc_s or stdlibc++.
            #
            # For more details, see the files under toolchains/clang.
            "https://github.com/retone/deps/releases/download/na5/clang+llvm-10.0.1-x86_64-pc-linux-gnu.tar.xz",
        ],
    },
    # Disabled for now waiting on https://github.com/pybind/pybind11_bazel/pull/29
    # sysroot = {
    #     "linux": "@org_chromium_sysroot_linux_x64//:sysroot",
    # },
)

maybe(
    http_archive,
    name = "rules_7zip",
    sha256 = "fd9e99f6ccb9e946755f9bc444abefbdd1eedb32c372c56dcacc7eb486aed178",
    strip_prefix = "rules_7zip-e00b15d3cb76b78ddc1c15e7426eb1d1b7ddaa3e",
    urls = ["https://github.com/zaucy/rules_7zip/archive/e00b15d3cb76b78ddc1c15e7426eb1d1b7ddaa3e.zip"],
)

load("@rules_7zip//:setup.bzl", "setup_7zip")

setup_7zip()

maybe(
    http_archive,
    name = "bazel_features",
    sha256 = "ba1282c1aa1d1fffdcf994ab32131d7c7551a9bc960fbf05f42d55a1b930cbfb",
    strip_prefix = "bazel_features-1.15.0",
    url = "https://github.com/bazel-contrib/bazel_features/releases/download/v1.15.0/bazel_features-v1.15.0.tar.gz",
)

load("@bazel_features//:deps.bzl", "bazel_features_deps")

bazel_features_deps()

maybe(
    http_archive,
    name = "rules_proto",
    sha256 = "6fb6767d1bef535310547e03247f7518b03487740c11b6c6adb7952033fe1295",
    strip_prefix = "rules_proto-6.0.2",
    url = "https://github.com/bazelbuild/rules_proto/releases/download/6.0.2/rules_proto-6.0.2.tar.gz",
)

#XX
maybe(
    http_archive,
    name = "rules_pkg",
    sha256 = "a89e203d3cf264e564fcb96b6e06dd70bc0557356eb48400ce4b5d97c2c3720d",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.5.1/rules_pkg-0.5.1.tar.gz",
        "https://github.com/bazelbuild/rules_pkg/releases/download/0.5.1/rules_pkg-0.5.1.tar.gz",
    ],
)

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")

rules_pkg_dependencies()

load("@rules_pkg//toolchains:rpmbuild.bzl", "rpmbuild_register_toolchains")

rpmbuild_register_toolchains()

load("@rules_python//python:pip.bzl", "pip_parse")

# Used only by the rules that vendor requirements.bzl
# Not needed by users of rules_hdl.
pip_parse(
    name = "rules_hdl_pip_deps_to_vendor",
    python_interpreter_target = "@python39_host//:python",
    requirements_lock = "//dependency_support:pip_requirements.txt",
)
# END XX

# maliberty:temp-abc-undo-revert
# until the abc update can be upstreamed without breaking xls
rules_hdl_git_hash = "ea34e431d6dbe0ba5ade73b4326d70d730ee35ef"
rules_hdl_git_sha256 = "363405fe710f71d380e5a3c87f2b40bad64bf7afcf3c257aa164333c49062cbc"

maybe(
    http_archive,
    name = "rules_hdl",
    sha256 = rules_hdl_git_sha256,
    strip_prefix = "bazel_rules_hdl-%s" % rules_hdl_git_hash,
    urls = [
#        "https://github.com/hdl/bazel_rules_hdl/archive/%s.tar.gz" % rules_hdl_git_hash,
        "https://github.com/maliberty/bazel_rules_hdl/archive/%s.tar.gz" % rules_hdl_git_hash,
    ],
)

load("@rules_hdl//dependency_support:dependency_support.bzl", rules_hdl_dependency_support = "dependency_support")
rules_hdl_dependency_support()

load("@rules_hdl//:init.bzl", rules_hdl_init = "init")
rules_hdl_init()
