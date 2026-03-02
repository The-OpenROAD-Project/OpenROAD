#include "utl/ScopedTemporaryFile.h"

#include <stdio.h>   // NOLINT(modernize-deprecated-headers): for fdopen()
#include <stdlib.h>  // NOLINT(modernize-deprecated-headers): for mkstemp()
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <ios>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

#include "boost/algorithm/string/predicate.hpp"
#include "boost/iostreams/filter/gzip.hpp"
#include "utl/Logger.h"
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

OutStreamHandler::OutStreamHandler(const char* filename, bool binary)
    : filename_(filename)
{
  if (filename_.empty()) {
    throw std::runtime_error("filename is empty");
  }
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

  if (boost::ends_with(filename_, ".gz")) {
    buf_ = std::make_unique<boost::iostreams::filtering_ostreambuf>();

    buf_->push(boost::iostreams::gzip_compressor());
    buf_->push(os_);

    stream_ = std::make_unique<std::ostream>(buf_.get());
  }
}

OutStreamHandler::~OutStreamHandler()
{
  try {
    close();
  } catch (const std::exception& e) {
    std::cerr << "Error: close for " << filename_ << " failed: " << e.what()
              << "\n";
  } catch (...) {
    std::cerr << "Error: close for " << filename_
              << " failed with unknown error\n";
  }
}

void OutStreamHandler::close()
{
  if (stream_) {
    boost::iostreams::close(*buf_);
    buf_ = nullptr;
    stream_ = nullptr;
  }

  if (os_.is_open()) {
    // Any pending output sequence is written to the file.
    os_.close();
    // If filename_ exists it will be overwritten
    fs::rename(tmp_filename_, filename_);
  }
}

std::ostream& OutStreamHandler::getStream()
{
  if (stream_) {
    return *stream_;
  }
  return os_;
}

InStreamHandler::InStreamHandler(const char* filename, bool binary)
    : filename_(filename)
{
  is_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  std::ios_base::openmode mode = std::ios_base::in;
  if (binary) {
    mode |= std::ios::binary;
  }
  try {
    is_.open(filename_, mode);
  } catch (std::ios_base::failure& e) {
    std::throw_with_nested(
        std::runtime_error("Failed to open '" + filename_ + "' for reading"));
  }

  if (boost::ends_with(filename_, ".gz")) {
    buf_ = std::make_unique<boost::iostreams::filtering_istreambuf>();

    buf_->push(boost::iostreams::gzip_decompressor());
    buf_->push(is_);

    stream_ = std::make_unique<std::istream>(buf_.get());
  }
}

InStreamHandler::~InStreamHandler()
{
  try {
    close();
  } catch (const std::exception& e) {
    std::cerr << "Error: close for " << filename_ << " failed: " << e.what()
              << "\n";
  } catch (...) {
    std::cerr << "Error: close for " << filename_
              << " failed with unknown error\n";
  }
}

void InStreamHandler::close()
{
  if (stream_) {
    boost::iostreams::close(*buf_);
    buf_ = nullptr;
    stream_ = nullptr;
  }

  if (is_.is_open()) {
    is_.close();
  }
}

std::istream& InStreamHandler::getStream()
{
  if (stream_) {
    return *stream_;
  }
  return is_;
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
  try {
    close();
  } catch (const std::exception& e) {
    std::cerr << "Error: close for " << filename_ << " failed: " << e.what()
              << "\n";
  } catch (...) {
    std::cerr << "Error: close for " << filename_
              << " failed with unknown error\n";
  }
}

void FileHandler::close()
{
  if (file_) {
    const bool failed = (std::fclose(file_) != 0);
    file_ = nullptr;
    if (failed) {
      throw std::runtime_error("fclose failed for " + tmp_filename_ + ": "
                               + strerror(errno));
    }
    // If filename_ exists it will be overwritten.
    fs::rename(tmp_filename_, filename_);
  }
}

FILE* FileHandler::getFile()
{
  return file_;
}

}  // namespace utl
