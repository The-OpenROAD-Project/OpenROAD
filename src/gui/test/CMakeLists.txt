if (BUILD_GUI)
  or_integration_tests(
    "gui"
    TESTS
      screenshot
  )
endif()

or_integration_tests(
  "gui"
  TESTS
    supported
)

# Skipped
#gui_man_tcl_check
#gui_readme_msgs_check
