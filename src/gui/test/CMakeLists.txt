set(GUI_TESTS supported)

if (Qt5_FOUND AND BUILD_GUI)
  list(APPEND GUI_TESTS dump_heatmap_headless)

  add_executable(gui_heatmap_sync_test heatmap_sync_test.cpp)
  target_link_libraries(gui_heatmap_sync_test
    gui
    odb
    utl_lib
    GTest::gtest
    GTest::gtest_main
  )
  gtest_discover_tests(gui_heatmap_sync_test)
endif()

or_integration_tests(
  "gui"
  TESTS
    ${GUI_TESTS}
)
