#include "utl/ScopedTemporaryFile.h"

#include <unistd.h>

#include <filesystem>
namespace fs = std::filesystem;

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

StreamHandler::StreamHandler(const char* filename,
                             std::ios_base::iostate flag,
                             std::ios_base::openmode mode)
    : filename_(filename), flag_(flag), mode_(mode)
{
  if (fs::exists(filename_)) {
    fs::remove(filename_);
  }
  std::string tmp_filename = filename_ + ".tmp";
  if (fs::exists(tmp_filename)) {
    fs::remove(tmp_filename);
  }
  os_.exceptions(flag | std::ofstream::failbit);
  os_.open(tmp_filename, mode);
}

StreamHandler::~StreamHandler()
{
  if (os_.is_open()) {
    os_.close();
  }
  std::string tmp_filename = filename_ + ".tmp";
  fs::rename(tmp_filename, filename_);
}

std::ofstream& StreamHandler::getStream()
{
  return os_;
}

FileHandler::FileHandler(const char* filename, const char* mode)
    : filename_(filename), mode_(mode)
{
  if (fs::exists(filename_)) {
    fs::remove(filename_);
  }
  std::string tmp_filename = filename_ + ".tmp";
  file = fopen(tmp_filename.c_str(), mode);
  if (!file) {
    throw std::runtime_error("Failed to open temporary file");
  }
}

FileHandler::~FileHandler()
{
  if (file_) {
    fclose(file_);
  }
  std::string tmp_filename = filename_ + ".tmp";
  fs::rename(tmp_filename, filename_);
}

FILE* FileHandler::getFile()
{
  return file_;
}

}  // namespace utl
