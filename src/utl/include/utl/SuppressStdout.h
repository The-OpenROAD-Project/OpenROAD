#include <fcntl.h>
#include <unistd.h>

#include "utl/Logger.h"

namespace utl {

// Redirects stdout to /dev/null while in scope
class SuppressStdout
{
 public:
  SuppressStdout(Logger* logger);
  ~SuppressStdout();

  SuppressStdout(SuppressStdout const&) = delete;
  SuppressStdout(SuppressStdout&&) = delete;

 private:
  int saved_fd_ = -1;
  Logger* logger_ = nullptr;
};

}  // namespace utl
