set(GUI_TESTS supported)
set(GUI_PASSFAIL_TESTS
  dump_heatmap_headless
  no_window
  select_name_patterns
)

if (Qt5_FOUND AND BUILD_GUI)
  list(APPEND GUI_PASSFAIL_TESTS clocktree_virtual_clock)
  list(APPEND GUI_PASSFAIL_TESTS display_controls_parity)
  list(APPEND GUI_PASSFAIL_TESTS save_animated_gif_headless)
  list(APPEND GUI_PASSFAIL_TESTS save_image_headless)
endif()

or_integration_tests(
  "gui"
  TESTS
    ${GUI_TESTS}
  PASSFAIL_TESTS
    ${GUI_PASSFAIL_TESTS}
)
