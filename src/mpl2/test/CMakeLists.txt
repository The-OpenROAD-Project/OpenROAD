or_integration_tests(
  "mpl2"
  TESTS
    macro_only
    no_unfixed_macros
    guides1
    guides2
)

# Skipped
#mpl2_man_tcl_check
#mpl2_readme_msgs_check

add_subdirectory(cpp)
