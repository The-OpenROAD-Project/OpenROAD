include("openroad")

set(TEST_NAMES
  simple01
  simple01-obs
  simple01-td
  simple01-td-tune
  simple01-uniform
  simple01-ref
  simple01-skip-io
  simple01-rd
  simple02-rd
  simple02
  simple03
  simple04
  simple05
  simple06
  simple07
  simple08
  simple09
  core01
  ar01
  ar02
  incremental01
  incremental02
  error01
  diverge01
  density01
  convergence01
  nograd01
  clust01
#  clust02
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
  or_integration_test("gpl" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

add_executable(fft_test fft_test.cc)

target_include_directories(fft_test
  PUBLIC
  ${PROJECT_SOURCE_DIR}
)

target_link_libraries(fft_test
  gtest
  gtest_main
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


add_dependencies(build_and_test fft_test)
