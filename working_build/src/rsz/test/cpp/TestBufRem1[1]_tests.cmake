add_test([=[BufRemTest.SlackImproves]=]  /home/memzfs_projects/MLBuf_extension/OR_latest/build/src/rsz/test/cpp/TestBufRem1 [==[--gtest_filter=BufRemTest.SlackImproves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BufRemTest.SlackImproves]=]  PROPERTIES WORKING_DIRECTORY /home/memzfs_projects/MLBuf_extension/OR_latest/src/rsz/test/cpp/.. SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  TestBufRem1_TESTS BufRemTest.SlackImproves)
