set(GUI_TESTS supported)
set(GUI_PASSFAIL_TESTS)

if (Qt5_FOUND AND BUILD_GUI)
  list(APPEND GUI_PASSFAIL_TESTS dump_heatmap_headless)
endif()

if (GUI_PASSFAIL_TESTS)
  or_integration_tests(
    "gui"
    TESTS
      ${GUI_TESTS}
    PASSFAIL_TESTS
      ${GUI_PASSFAIL_TESTS}
  )
else()
  or_integration_tests(
    "gui"
    TESTS
      ${GUI_TESTS}
  )
endif()
