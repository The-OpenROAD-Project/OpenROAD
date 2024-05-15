#include "utl/ScopedTemporaryFile.h"

#include <unistd.h>

namespace utl {

ScopedTemporaryFile::ScopedTemporaryFile(Logger* logger) : logger_(logger)
{
  strncpy(path_, "/tmp/openroad_CFileUtilsTest_XXXXXX", sizeof(path_) - 1);
  fd_ = mkstemp(path_);
  if (fd_ < 0) {
    logger_->error(UTL, 5, "could not create temp file");
  }

  logger_->info(UTL, 6, "ScopedTemporaryFile; fd: {} path: {}", fd_, path_);

  file_ = fdopen(fd_, "w+");
  if (file_ == nullptr) {
    logger_->error(UTL, 7, "could not open temp descriptor as FILE");
  }
}

ScopedTemporaryFile::~ScopedTemporaryFile()
{
  if (fd_ >= 0) {
    if (unlink(path_) < 0) {
      logger_->warn(UTL, 8, "could not unlink temp file at {}", path_);
    }
    if (fclose(file_) < 0) {
      std::string error = strerror(errno);
      logger_->warn(UTL, 9, "could not close temp file: {}", error);
    }
  }
}

std::string createTmpFileName(const char* filename)
{
  std::time_t now = std::time(nullptr);
  std::tm* tm = std::gmtime(&now);
  char timestamp[20];
  std::strftime(timestamp, sizeof(timestamp), "%Y%m%dT%H%M%SZ", tm);

  fs::path filepath = filename;
  fs::path directory = filepath.parent_path();
  std::string base_filename = filepath.filename().string();
  // Create temporary filename with timestamp in /tmp directory
  fs::path tmp_dir = fs::temp_directory_path();
  return  (tmp_dir / (std::string(timestamp) + "_" + base_filename)).string();
}

}  // namespace utl
