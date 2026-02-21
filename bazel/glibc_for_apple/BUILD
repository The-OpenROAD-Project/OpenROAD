package(
    default_visibility = ["//visibility:public"],
)

# Match glibc functions for platform stable results. The functions are
# only used on MacOS as linux systems provide these functions by glibc
# Portable rand()/srand() and qsort() implementation matching glibc's algorithm.
# Apple's libc rand() produces different sequences for the same seed,
# causing non-determinism in code using rand() across platforms (e.g., ABC).
# qsort() from glibc and Apple differ for equal elements causing also
# non-determinism.
cc_library(
    name = "portable_rand_qsort",
    srcs = select({
        # On macOS, override rand()/srand() and qsort() to match glibc's
        # algorithms. Apple's libc uses different algorithms for both,
        # causing non-determinism in code like ABC across platforms.
        # On Linux (glibc), these are already the native implementations.
        "@platforms//os:macos": [
            "portable_rand.c",
            "portable_qsort.c",
        ],
        "//conditions:default": [],
    }),
    copts = [
        "-std=c99",
        "-fno-builtin",
        "-w",
    ],
    alwayslink = True,
)
