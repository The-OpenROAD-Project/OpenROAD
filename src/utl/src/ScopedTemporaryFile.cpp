#include "utl/ScopedTemporaryFile.h"

#include <unistd.h>

#include <filesystem>
namespace fs = std::filesystem;

namespace utl {

namespace {
std::string generate_unused_filename(const std::string& prefix)
{
  int counter = 1;
  std::string filename;
  do {
    filename = fmt::format("{}.{}", prefix, counter++);
  } while (std::filesystem::exists(filename));

  return filename;
}
}  // namespace

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
  tmp_filename_ = generate_unused_filename(filename_);

  os_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  std::ios_base::openmode mode = std::ios_base::out | std::ios::trunc;
  if (binary) {
    mode |= std::ios::binary;
  }
  try {
    os_.open(tmp_filename_, mode);
  } catch (std::ios_base::failure& e) {
    std::throw_with_nested(std::runtime_error("Failed to open '" + tmp_filename_
                                              + "' for writing"));
  }
}

StreamHandler::~StreamHandler()
{
  if (os_.is_open()) {
    // Any pending output sequence is written to the file.
    os_.close();
  }
  // If filename_ exists it will be overwritten
  fs::rename(tmp_filename_, filename_);
}

std::ofstream& StreamHandler::getStream()
{
  return os_;
}

FileHandler::FileHandler(const char* filename, bool binary)
    : filename_(filename)
{
  tmp_filename_ = generate_unused_filename(filename_);
  file_ = fopen(tmp_filename_.c_str(), (binary ? "wb" : "w"));
  if (!file_) {
    std::string error = strerror(errno);
    throw std::runtime_error(error + ": " + tmp_filename_);
  }
}

FileHandler::~FileHandler()
{
  if (file_) {
    // Any unwritten buffered data are flushed to the OS.
    std::fclose(file_);
  }
  // If filename_ exists it will be overwritten.
  fs::rename(tmp_filename_, filename_);
}

FILE* FileHandler::getFile()
{
  return file_;
}

}  // namespace utl
