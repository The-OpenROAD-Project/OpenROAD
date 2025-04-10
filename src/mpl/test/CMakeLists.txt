or_integration_tests(
  "mpl"
  TESTS
    macro_only
    no_unfixed_macros
    guides1
    guides2
    io_constraints1
    io_constraints2
    io_pads1
    orientation_improve1
    orientation_improve2
    boundary_push1
    boundary_push2
    boundary_push3
    centralization1
    clocked_macro
)

# Skipped
#mpl_man_tcl_check
#mpl_readme_msgs_check

add_subdirectory(cpp)
