#include "env.h"

#include <iostream>
#include <stdexcept>

namespace odb {

std::string testTmpPath(const std::string& path, const std::string& file)
{
  const char* base_dir = std::getenv("BASE_DIR");
  if (!base_dir) {
    const char* pwd = std::getenv("PWD");
    if (!pwd) {
      throw std::runtime_error(
          "BASE_DIR and PWD environment variables are not set");
    }
    return std::string(pwd) + file;
  }
  return std::string(base_dir) + "/" + path + "/" + file;
}

}  // namespace odb