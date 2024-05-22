include(openroad)

add_executable(TestBufRem1 TestBufferRemoval.cc)
target_link_libraries(TestBufRem1
        OpenSTA
        gtest
        gmock
        gtest_main
        dbSta_lib
        utl_lib
        rsz_lib
        grt_lib
        dpl_lib
        stt_lib
        ${TCL_LIBRARY}
)

target_include_directories(TestBufRem1
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/rsz/src
)

add_dependencies(build_and_test TestBufRem1
)
