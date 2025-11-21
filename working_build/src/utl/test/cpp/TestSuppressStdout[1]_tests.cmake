add_test([=[Utl.SuppressStdout]=]  /home/memzfs_projects/MLBuf_extension/OR_latest/build/src/utl/test/cpp/TestSuppressStdout [==[--gtest_filter=Utl.SuppressStdout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Utl.SuppressStdout]=]  PROPERTIES WORKING_DIRECTORY /home/memzfs_projects/MLBuf_extension/OR_latest/src/utl/test/cpp SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  TestSuppressStdout_TESTS Utl.SuppressStdout)
