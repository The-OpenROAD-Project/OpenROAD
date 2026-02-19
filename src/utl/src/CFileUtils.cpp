#include "utl/CFileUtils.h"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>

#include "utl/Logger.h"

namespace utl {

std::string GetContents(FILE* file, Logger* logger)
{
  if (fseek(file, 0, SEEK_SET) != 0) {
    logger->error(UTL, 1, "seeking file to start {}", errno);
  }

  std::string result;

  // Read bytes into this buffer each loop trip before placing it into the
  // result string.
  //
  // (The total amount successfully read is result.size().)
  constexpr size_t kBufSize = 1024;
  std::array<uint8_t, kBufSize> buf;

  while (true) {
    size_t read = fread(buf.data(), 1, kBufSize, file);
    if (read == 0) {
      if (ferror(file)) {
        std::string error = strerror(errno);
        logger->error(UTL,
                      2,
                      "error reading {} bytes from file at offset {}: {}",
                      kBufSize,
                      result.size(),
                      error);
      }
      if (feof(file)) {
        break;
      }
      logger->error(
          UTL,
          3,
          "read no bytes from file at offset {}, but neither error nor EOF",
          result.size());
    }
    result.append(buf.begin(), buf.begin() + read);
  }

  return result;
}

void WriteAll(FILE* file, std::span<const uint8_t> data, Logger* logger)
{
  size_t total_written = 0;
  while (total_written < data.size()) {
    size_t this_size = data.size() - total_written;
    size_t written = fwrite(data.data() + total_written, 1, this_size, file);
    if (written == 0) {
      if (ferror(file)) {
        std::string error = strerror(errno);
        logger->error(UTL,
                      4,
                      "error writing {} bytes from file at offset {}: {}",
                      this_size,
                      total_written,
                      error);
      }
    }
    total_written += written;
  }
}

}  // namespace utl
