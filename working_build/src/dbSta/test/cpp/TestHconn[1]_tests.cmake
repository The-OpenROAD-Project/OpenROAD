add_test([=[TestHconn.ConnectionMade]=]  /home/memzfs_projects/MLBuf_extension/OR_latest/build/src/dbSta/test/cpp/TestHconn [==[--gtest_filter=TestHconn.ConnectionMade]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TestHconn.ConnectionMade]=]  PROPERTIES WORKING_DIRECTORY /home/memzfs_projects/MLBuf_extension/OR_latest/src/dbSta/test/cpp/.. SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  TestHconn_TESTS TestHconn.ConnectionMade)
