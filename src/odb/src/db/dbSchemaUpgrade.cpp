// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "odb/dbSchemaUpgrade.h"

#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <crt_externs.h>
#include <mach-o/dyld.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <istream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

#include "dbDatabase.h"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"

#if !defined(__APPLE__)
extern char** environ;
#endif

namespace odb {

namespace {

// dbIStream refills in 64 KiB chunks, so it can over-read, and on
// destruction it rewinds whatever it buffered but did not consume -- putting
// the underlying stream back where the next reader expects it. A pipe cannot
// seek backwards, so the reader below retains a window of already-delivered
// bytes and serves such a rewind from it.
//
// This is defensive rather than load-bearing. A converter writes exactly one
// database and then closes, so the final refill returns only the bytes that
// remain and all of them are consumed: measured against the schema 57 and 98
// databases in the tree, the rewind never fires and seekoff is never called.
// It would fire on a stream carrying anything after the database. Handling
// that is worth the window because the alternative is not a failed read: the
// stream runs with failbit exceptions enabled, so a refused seek would throw
// from ~dbIStream and terminate the process.
//
// The rewind can never exceed dbIStream's own buffer, so 64 KiB is the true
// bound; the window is four times that purely so a future change to that
// buffer does not quietly turn a seek into a crash.
constexpr size_t kRewindWindow = 256 * 1024;
constexpr size_t kReadChunk = 256 * 1024;

// A read-only streambuf over a file descriptor that supports relative
// backward seeks within kRewindWindow bytes.
class RewindablePipeBuf : public std::streambuf
{
 public:
  explicit RewindablePipeBuf(int fd)
      : fd_(fd), buffer_(kRewindWindow + kReadChunk)
  {
    char* base = buffer_.data();
    setg(base, base, base);
  }

 protected:
  int_type underflow() override
  {
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    // Keep the tail of what has already been delivered so a later rewind
    // can be served without going back to the descriptor.
    char* base = buffer_.data();
    const size_t delivered = static_cast<size_t>(gptr() - eback());
    const size_t keep = std::min(delivered, kRewindWindow);
    std::memmove(base, gptr() - keep, keep);
    base_pos_ += delivered - keep;

    // One short read is fine: the caller comes back through underflow() for
    // whatever is left.
    ssize_t n = 0;
    do {
      n = ::read(fd_, base + keep, kReadChunk);
    } while (n < 0 && errno == EINTR);
    const size_t filled = n > 0 ? static_cast<size_t>(n) : 0;

    setg(base, base + keep, base + keep + filled);
    if (filled == 0) {
      return traits_type::eof();
    }
    return traits_type::to_int_type(*gptr());
  }

  pos_type seekoff(off_type off,
                   std::ios_base::seekdir dir,
                   std::ios_base::openmode which) override
  {
    if ((which & std::ios_base::in) == 0) {
      return pos_type(off_type(-1));
    }

    char* target = nullptr;
    if (dir == std::ios_base::cur) {
      target = gptr() + off;
    } else if (dir == std::ios_base::beg) {
      target = eback() + (off - static_cast<off_type>(base_pos_));
    } else {
      return pos_type(off_type(-1));
    }

    if (target < eback() || target > egptr()) {
      return pos_type(off_type(-1));
    }
    setg(eback(), target, egptr());
    return pos_type(static_cast<off_type>(base_pos_) + (target - eback()));
  }

  pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
  {
    return seekoff(off_type(pos), std::ios_base::beg, which);
  }

 private:
  int fd_;
  std::vector<char> buffer_;
  // Absolute file offset of eback(), so seekoff can report a position.
  size_t base_pos_ = 0;
};

// Deliberately not isdigit(): these characters come from an arbitrary file
// name in a scanned directory, so a byte above 127 is reachable and would be
// a negative int on a signed-char platform, which isdigit() does not accept.
// A version number is ASCII regardless of locale anyway.
bool isAsciiDigits(const std::string& s)
{
  return !s.empty() && std::all_of(s.begin(), s.end(), [](const char c) {
    return c >= '0' && c <= '9';
  });
}

// Parses "convert-<from>-<to>"; anything else in the directory is ignored.
std::optional<SchemaConverter> parseConverterName(
    const std::filesystem::path& path)
{
  const std::string name = path.filename().string();
  if (name.rfind("convert-", 0) != 0) {
    return std::nullopt;
  }
  const std::string rest = name.substr(std::strlen("convert-"));
  const size_t dash = rest.find('-');
  if (dash == std::string::npos) {
    return std::nullopt;
  }

  try {
    const std::string from = rest.substr(0, dash);
    const std::string to = rest.substr(dash + 1);
    if (!isAsciiDigits(from) || !isAsciiDigits(to)) {
      return std::nullopt;
    }
    return SchemaConverter{path.string(),
                           static_cast<uint32_t>(std::stoul(from)),
                           static_cast<uint32_t>(std::stoul(to))};
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

// macOS has no /proc, and its environ is not linkable from a shared library,
// so both of the process-introspection bits below need a second spelling.
char** currentEnviron()
{
#if defined(__APPLE__)
  return *_NSGetEnviron();
#else
  return environ;
#endif
}

std::filesystem::path selfExecutable()
{
  std::error_code ec;
#if defined(__APPLE__)
  uint32_t size = 0;
  // The first call fails and reports the size it needs.
  _NSGetExecutablePath(nullptr, &size);
  std::string buf(size, '\0');
  if (_NSGetExecutablePath(buf.data(), &size) != 0) {
    return {};
  }
  buf.resize(std::strlen(buf.c_str()));
  // _NSGetExecutablePath may hand back a path with symlinks or .. in it;
  // /proc/self/exe is already resolved, so resolve here to match.
  const std::filesystem::path exe = std::filesystem::canonical(buf, ec);
  return ec ? std::filesystem::path(buf) : exe;
#else
  const std::filesystem::path exe
      = std::filesystem::read_symlink("/proc/self/exe", ec);
  return ec ? std::filesystem::path() : exe;
#endif
}

std::vector<std::filesystem::path> converterSearchPath()
{
  std::vector<std::filesystem::path> dirs;

  // An explicit override wins, so a converter can be pointed at without
  // reinstalling.
  if (const char* env = std::getenv("ORD_ODB_CONVERTERS")) {
    std::string spec(env);
    size_t start = 0;
    while (start <= spec.size()) {
      const size_t sep = spec.find(':', start);
      const std::string dir = spec.substr(
          start, sep == std::string::npos ? std::string::npos : sep - start);
      if (!dir.empty()) {
        dirs.emplace_back(dir);
      }
      if (sep == std::string::npos) {
        break;
      }
      start = sep + 1;
    }
  }

  // Under `bazel test` the binary and its data share one runfiles tree, so
  // the package-relative path below is the one that resolves.
  const std::filesystem::path kPackage
      = std::filesystem::path("src") / "odb" / "converter";

  for (const char* var : {"RUNFILES_DIR", "TEST_SRCDIR"}) {
    if (const char* root = std::getenv(var)) {
      dirs.push_back(std::filesystem::path(root) / "_main" / kPackage);
    }
  }

  const std::filesystem::path exe = selfExecutable();
  if (!exe.empty()) {
    // An installed tree: converters sit next to the binary.
    dirs.push_back(exe.parent_path());
    // bazel-bin, and the _main directory of a runfiles tree: both put the
    // converters under the binary's directory at their package path.
    dirs.push_back(exe.parent_path() / kPackage);
    // `bazel run`, where the binary has its own runfiles directory.
    dirs.push_back(exe.parent_path() / (exe.filename().string() + ".runfiles")
                   / "_main" / kPackage);
  }

  return dirs;
}

}  // namespace

std::optional<DbFileHeader> peekDbFileHeader(const std::string& filename)
{
  try {
    // Same opener the reader uses, so a .gz database is peeked the same way
    // it is read.
    utl::InStreamHandler handler(filename.c_str(), true);
    std::istream& in = handler.getStream();

    uint32_t words[4] = {0, 0, 0, 0};
    in.read(reinterpret_cast<char*>(words), sizeof(words));
    if (in.gcount() != static_cast<std::streamsize>(sizeof(words))) {
      return std::nullopt;
    }
    if (words[0] != kMagic1 || words[1] != kMagic2) {
      return std::nullopt;
    }
    return DbFileHeader{words[2], words[3]};
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

std::vector<SchemaConverter> findSchemaConverters()
{
  std::vector<SchemaConverter> found;
  for (const std::filesystem::path& dir : converterSearchPath()) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
      continue;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
      if (ec) {
        break;
      }
      if (std::optional<SchemaConverter> c = parseConverterName(entry.path())) {
        if (::access(c->path.c_str(), X_OK) == 0) {
          found.push_back(*c);
        }
      }
    }
  }
  return found;
}

class DbFileStream::Impl
{
 public:
  Impl(const std::string& filename, utl::Logger* logger)
      : filename_(filename), logger_(logger)
  {
    const std::optional<DbFileHeader> header = peekDbFileHeader(filename);

    // Not a database, or unreadable: hand it to the normal reader, which
    // owns the error message for that case.
    if (!header || header->schema_minor >= kSchemaOldestReadable) {
      direct_ = std::make_unique<utl::InStreamHandler>(filename.c_str(), true);
      return;
    }

    startConverter(header->schema_minor);
  }

  ~Impl()
  {
    if (pid_ != -1) {
      // finish() was not reached, e.g. the read threw. Do not let the child
      // outlive us holding a pipe.
      ::kill(pid_, SIGTERM);
      int status = 0;
      ::waitpid(pid_, &status, 0);
    }
    if (read_fd_ != -1) {
      ::close(read_fd_);
    }
  }

  std::istream& stream()
  {
    if (direct_) {
      return direct_->getStream();
    }
    return *pipe_stream_;
  }

  void finish()
  {
    if (pid_ == -1) {
      return;
    }
    const pid_t pid = pid_;
    pid_ = -1;

    int status = 0;
    while (::waitpid(pid, &status, 0) == -1) {
      if (errno != EINTR) {
        throw std::runtime_error("waiting for .odb converter failed");
      }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      throw std::runtime_error("the .odb converter " + converter_path_
                               + " failed; see its messages above");
    }
  }

 private:
  void startConverter(uint32_t file_schema)
  {
    const std::vector<SchemaConverter> converters = findSchemaConverters();

    // A converter reads a closed range and writes its own revision, so the
    // usable ones are those covering this file and landing somewhere this
    // build accepts. Prefer the smallest such target: the least conversion
    // that does the job.
    const SchemaConverter* best = nullptr;
    for (const SchemaConverter& c : converters) {
      if (file_schema < c.reads_from || file_schema > c.writes) {
        continue;
      }
      if (c.writes < kSchemaOldestReadable || c.writes > kSchemaMinor) {
        continue;
      }
      if (best == nullptr || c.writes < best->writes) {
        best = &c;
      }
    }

    if (best == nullptr) {
      std::string available;
      for (const SchemaConverter& c : converters) {
        available += " convert-" + std::to_string(c.reads_from) + "-"
                     + std::to_string(c.writes);
      }
      if (available.empty()) {
        available = " (none found)";
      }
      throw std::runtime_error(
          "database " + filename_ + " uses schema "
          + std::to_string(file_schema) + ", older than the oldest schema "
          + std::to_string(kSchemaOldestReadable)
          + " this build reads. No converter can bring it forward; converters"
            " found:"
          + available);
    }

    converter_path_ = best->path;
    // Only the base name, and not the converter's path at all: this line
    // lands in test golden files, and both the database and the converter are
    // routinely named by absolute paths that differ per machine and per build
    // tree. The full paths go to the debug stream instead.
    logger_->info(utl::ODB,
                  535,
                  "Converting {} from schema {} to schema {}.",
                  std::filesystem::path(filename_).filename().string(),
                  file_schema,
                  best->writes);
    debugPrint(logger_,
               utl::ODB,
               "schema",
               1,
               "converting {} with {}",
               filename_,
               converter_path_);

    int fds[2];
    if (::pipe(fds) != 0) {
      throw std::runtime_error("cannot create a pipe for the .odb converter");
    }

    // Checked because an unnoticed failure here does not fail the spawn: the
    // child would start with the wrong descriptors and write the database
    // somewhere other than the pipe, which surfaces much later as a truncated
    // read. init failing is the sharper case -- destroy() and the add calls
    // would then be operating on an uninitialized object.
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
      ::close(fds[0]);
      ::close(fds[1]);
      throw std::runtime_error(
          "cannot set up file actions for the .odb converter");
    }
    // The child writes the converted database on stdout; its diagnostics stay
    // on stderr and reach the user unchanged.
    if (posix_spawn_file_actions_addclose(&actions, fds[0]) != 0
        || posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO)
               != 0
        || posix_spawn_file_actions_addclose(&actions, fds[1]) != 0) {
      posix_spawn_file_actions_destroy(&actions);
      ::close(fds[0]);
      ::close(fds[1]);
      throw std::runtime_error(
          "cannot set up file actions for the .odb converter");
    }

    std::string path_arg = converter_path_;
    std::string file_arg = filename_;
    char* argv[] = {path_arg.data(), file_arg.data(), nullptr};

    pid_t pid = -1;
    const int rc = posix_spawn(&pid,
                               converter_path_.c_str(),
                               &actions,
                               nullptr,
                               argv,
                               currentEnviron());
    posix_spawn_file_actions_destroy(&actions);
    ::close(fds[1]);

    if (rc != 0) {
      ::close(fds[0]);
      throw std::runtime_error("cannot run the .odb converter "
                               + converter_path_ + ": " + std::strerror(rc));
    }

    pid_ = pid;
    read_fd_ = fds[0];
    pipe_buf_ = std::make_unique<RewindablePipeBuf>(read_fd_);
    pipe_stream_ = std::make_unique<std::istream>(pipe_buf_.get());
    // Matches utl::InStreamHandler: odb relies on failbit exceptions to turn
    // a truncated stream into an error instead of silent garbage.
    pipe_stream_->exceptions(std::istream::failbit | std::istream::badbit);
  }

  std::string filename_;
  utl::Logger* logger_;

  std::unique_ptr<utl::InStreamHandler> direct_;

  std::string converter_path_;
  pid_t pid_ = -1;
  int read_fd_ = -1;
  std::unique_ptr<RewindablePipeBuf> pipe_buf_;
  std::unique_ptr<std::istream> pipe_stream_;
};

DbFileStream::DbFileStream(const std::string& filename, utl::Logger* logger)
    : impl_(std::make_unique<Impl>(filename, logger))
{
}

DbFileStream::~DbFileStream() = default;

std::istream& DbFileStream::stream()
{
  return impl_->stream();
}

void DbFileStream::finish()
{
  impl_->finish();
}

}  // namespace odb
