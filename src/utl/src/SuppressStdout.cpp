#include "utl/SuppressStdout.h"

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>

#include "utl/Logger.h"

namespace utl {

SuppressStdout::SuppressStdout(utl::Logger* logger) : logger_(logger)
{
  fflush(stdout);

  saved_fd_ = dup(STDOUT_FILENO);
  if (saved_fd_ < 0) {
    logger_->error(UTL, 73, "Failed to dup STDOUT_FILENO");
    return;
  }

  const int dev_null_fd = open("/dev/null", O_WRONLY);
  if (dev_null_fd < 0) {
    close(saved_fd_);
    saved_fd_ = -1;
    logger_->error(UTL, 74, "Failed to open /dev/null");
    return;
  }

  if (dup2(dev_null_fd, STDOUT_FILENO) < 0) {
    close(saved_fd_);
    saved_fd_ = -1;
    logger_->error(UTL, 75, "dup2 failed to redirect STDOUT_FILENO");
  }

  close(dev_null_fd);
}

SuppressStdout::~SuppressStdout()
{
  fflush(stdout);
  if (saved_fd_ == -1) {
    return;
  }

  if (dup2(saved_fd_, STDOUT_FILENO) < 0) {
    logger_->error(UTL,
                   76,
                   "dup2 failed to restore stdout, all stdout is likely broken "
                   "from this point forward.");
  } else {
    close(saved_fd_);
  }
}

}  // namespace utl
