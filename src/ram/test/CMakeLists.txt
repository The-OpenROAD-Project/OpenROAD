or_integration_tests(
  "ram"
  TESTS
    # make_8x8_sky130 is bazel-only: DRT routing is FP-sensitive and the
    # golden .defok matches the clang build used by bazel, not the gcc
    # build typically used by cmake. See src/ram/test/BUILD.
    make_7x7_nangate45
)

