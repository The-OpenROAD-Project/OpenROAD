load("@rules_scala//scala:scala_toolchain.bzl", "scala_toolchain")

# Custom Scala toolchain with SemanticDB enabled
scala_toolchain(
    name = "semanticdb_toolchain_impl",
    enable_semanticdb = True,
    semanticdb_bundle_in_jar = False,
    visibility = ["//visibility:public"],
)

toolchain(
    name = "semanticdb_toolchain",
    toolchain = ":semanticdb_toolchain_impl",
    toolchain_type = "@rules_scala//scala:toolchain_type",
)
