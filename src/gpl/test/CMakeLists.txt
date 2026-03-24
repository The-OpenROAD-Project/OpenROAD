or_integration_tests(
  "gpl"
  TESTS
    ar01
    ar02
    clust01
    clust02
    clust03
    cluster_place01
    convergence01
    core01
    density01
    diverge01
    error01
    incremental01
    incremental02
    nograd01
    simple01
    simple01-obs
    simple01-rd
    simple01-ref
    simple01-skip-io
    simple01-td
    simple01-td-tune
    simple01-uniform
    simple02
    simple02-rd
    simple03
    simple03-rd
    simple04
    simple04-rd
    simple05
    simple06
    simple07
    simple08
    simple09
    simple10
    region01
)


add_executable(fft_test fft_test.cc)

target_include_directories(fft_test
  PUBLIC
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(fft_test
  GTest::gtest
  GTest::gtest_main
  spdlog::spdlog
)

gtest_discover_tests(fft_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(fft_test
  PRIVATE
  fft_test.cc
  ../src/fft.cpp
  ../src/fftsg.cpp
  ../src/fftsg2d.cpp
)

add_executable(mbff_test 
  mbff_test.cpp
)

target_include_directories(mbff_test
  PRIVATE
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(mbff_test PUBLIC
  absl::strings
  GTest::gtest
  GTest::gtest_main
  gpl_lib
  tst
)

gtest_discover_tests(mbff_test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

target_sources(mbff_test
  PRIVATE
  mbff_test.cpp
)

add_dependencies(build_and_test fft_test mbff_test)
