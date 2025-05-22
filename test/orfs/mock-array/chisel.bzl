# buildifier: disable=module-docstring
load("@rules_scala//scala:scala.bzl", "scala_binary", "scala_library")

def chisel_binary(name, **kwargs):
    scala_binary(
        name = name,
        deps = [
            "@chisel//:chisel",
            "@chisel//:core",
            "@chisel//:macros",
            "@chisel//:firrtl",
            "@chisel//:svsim",
        ] + kwargs.pop("deps", []),
        scalacopts = [
            "-language:reflectiveCalls",
            "-deprecation",
            "-feature",
            "-Xcheckinit",
        ] + kwargs.pop("scalacopts", []),
        plugins = [
            "@chisel//:plugin",
        ],
        **kwargs
    )

def chisel_library(name, **kwargs):
    scala_library(
        name = name,
        deps = [
            "@chisel//:chisel",
            "@chisel//:core",
            "@chisel//:macros",
            "@chisel//:firrtl",
            "@chisel//:svsim",
        ] + kwargs.pop("deps", []),
        scalacopts = [
            "-language:reflectiveCalls",
            "-deprecation",
            "-feature",
            "-Xcheckinit",
        ] + kwargs.pop("scalacopts", []),
        plugins = [
            "@chisel//:plugin",
        ],
        **kwargs
    )
