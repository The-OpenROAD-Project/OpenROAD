# buildifier: disable=module-docstring
load("@rules_scala//scala:scala.bzl", "scala_binary", "scala_library")

def chisel_binary(name, **kwargs):
    scala_binary(
        name = name,
        deps = [
            "@openroad_maven//:org_chipsalliance_chisel_2_13",
        ] + kwargs.pop("deps", []),
        scalacopts = [
            "-language:reflectiveCalls",
            "-deprecation",
            "-feature",
            "-Xcheckinit",
        ] + kwargs.pop("scalacopts", []),
        plugins = [
            "@openroad_maven//:org_chipsalliance_chisel_plugin_2_13_16",
        ],
        **kwargs
    )

def chisel_library(name, **kwargs):
    scala_library(
        name = name,
        deps = [
            "@openroad_maven//:org_chipsalliance_chisel_2_13",
        ] + kwargs.pop("deps", []),
        scalacopts = [
            "-language:reflectiveCalls",
            "-deprecation",
            "-feature",
            "-Xcheckinit",
        ] + kwargs.pop("scalacopts", []),
        plugins = [
            "@openroad_maven//:org_chipsalliance_chisel_plugin_2_13_16",
        ],
        **kwargs
    )
