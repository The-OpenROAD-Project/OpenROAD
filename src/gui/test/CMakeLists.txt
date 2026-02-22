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

