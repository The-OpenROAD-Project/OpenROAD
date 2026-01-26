// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "utl/Logger.h"
#include "utl/SuppressStdout.h"

namespace utl {

TEST(Utl, SuppressStdout)
{
  Logger logger;

  // 1. Create a temporary directory managed by GTest
  auto temp_dir = testing::TempDir();
  std::string file_path = temp_dir + "/my_file.txt";

  // 3. Create the file and get the file descriptor (fd)
  int fd = open(file_path.c_str(), O_WRONLY | O_CREAT, 0600);
  ASSERT_NE(fd, -1);

  int saved_stdout = dup(STDOUT_FILENO);
  ASSERT_NE(saved_stdout, -1);

  ASSERT_NE(dup2(fd, STDOUT_FILENO), -1);

  fprintf(stdout, "Before suppression\n");
  {
    SuppressStdout so(&logger);
    fprintf(stdout, "During suppression\n");
  }

  fprintf(stdout, "After suppression\n");
  fflush(stdout);

  close(fd);
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);

  std::ifstream log_file(file_path);
  std::string line;

  bool before_found = false;
  bool during_found = false;
  bool after_found = false;

  while (std::getline(log_file, line)) {
    if (line == "Before suppression") {
      before_found = true;
    } else if (line == "During suppression") {
      during_found = true;
    } else if (line == "After suppression") {
      after_found = true;
    }
  }

  EXPECT_TRUE(before_found);
  EXPECT_FALSE(during_found);
  EXPECT_TRUE(after_found);
}

}  // namespace utl
