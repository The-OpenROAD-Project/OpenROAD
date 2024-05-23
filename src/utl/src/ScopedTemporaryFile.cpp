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

StreamHandler::StreamHandler(const char* filename, bool binary)
    : filename_(filename)
{
  std::string tmp_filename = filename_ + ".tmp";
  if (fs::exists(tmp_filename)) {
    fs::remove(tmp_filename);
  }
  os_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  try {
    os_.open(tmp_filename,
             (binary ? (std::ios::binary | std::ios_base::out)
                     : std::ios_base::out));
  } catch (std::ios_base::failure& e) {
    std::string error = e.what();
    std::throw_with_nested(std::ios_base::failure(error + " (failed to open '"
                                                  + tmp_filename + "')"));
  }
}

StreamHandler::~StreamHandler()
{
  if (os_.is_open()) {
    os_.close();
  }
  std::string tmp_filename = filename_ + ".tmp";
  // If filename_ exists it will be overwritten
  fs::rename(tmp_filename, filename_);
}

std::ofstream& StreamHandler::getStream()
{
  return os_;
}

FileHandler::FileHandler(const char* filename, bool binary)
    : filename_(filename)
{
  std::string tmp_filename = filename_ + ".tmp";
  file_ = fopen(tmp_filename.c_str(), (binary ? "wb" : "w"));
  if (!file_) {
    std::string error = strerror(errno);
    throw std::runtime_error(error + ": " + tmp_filename);
  }
}

FileHandler::~FileHandler()
{
  if (file_) {
    fclose(file_);
  }
  std::string tmp_filename = filename_ + ".tmp";
  // If filename_ exists it will be overwritten
  fs::rename(tmp_filename, filename_);
}

FILE* FileHandler::getFile()
{
  return file_;
}

}  // namespace utl
