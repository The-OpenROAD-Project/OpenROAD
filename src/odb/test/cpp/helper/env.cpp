#include "env.h"

#include <stdexcept>

std::string testTmpPath(const std::string& file)
{
  const char* base_dir = std::getenv("BASE_DIR");
  if (!base_dir) {
    throw std::runtime_error("BASE_DIR environment variable not set");
  }
  return std::string(base_dir) + file;
}
