// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "browser_launcher.h"

#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <crt_externs.h>
#endif

#include <cerrno>
#include <chrono>
#include <future>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace web {

namespace {

std::string errnoText(const std::string& who, const int err)
{
  return who + ": " + std::error_code(err, std::generic_category()).message();
}

char** launcherEnviron()
{
#ifdef __APPLE__
  // A dylib, as the macOS tests link this, cannot see `environ`.
  return *_NSGetEnviron();
#else
  return environ;
#endif
}

// What `pid`, running `program`, came to within `wait`: empty for a zero exit,
// and for one still running.
// NOLINTNEXTLINE(misc-include-cleaner): <spawn.h> declares pid_t too.
std::string awaitLauncher(const pid_t pid,
                          const std::string& program,
                          const std::chrono::milliseconds wait)
{
  // A wait status, or minus the errno of waitpid.
  std::promise<int> exited;
  std::future<int> result = exited.get_future();
  // Detached, so that a launcher that stays, as xdg-open does when it runs the
  // browser itself, holds up neither serve() nor a zombie.
  std::thread([pid, exited = std::move(exited)]() mutable {
    int status = 0;
    while (waitpid(pid, &status, 0) == -1) {
      if (errno != EINTR) {
        exited.set_value(-errno);
        return;
      }
    }
    exited.set_value(status);
  }).detach();

  if (result.wait_for(wait) != std::future_status::ready) {
    return {};
  }
  const int status = result.get();
  if (status < 0) {
    return errnoText(program, -status);
  }
  // NOLINTBEGIN(misc-include-cleaner): <sys/wait.h> defines these too.
  if (WIFSIGNALED(status)) {
    return program + " was killed by signal "
           + std::to_string(WTERMSIG(status));
  }
  if (WEXITSTATUS(status) != 0) {
    return program + " exited with status "
           + std::to_string(WEXITSTATUS(status));
  }
  // NOLINTEND(misc-include-cleaner)
  return {};
}

}  // namespace

std::string runBrowserLauncher(std::vector<std::string> argv,
                               const std::string& stderr_path,
                               const std::chrono::milliseconds wait)
{
  posix_spawn_file_actions_t actions;
  if (const int err = posix_spawn_file_actions_init(&actions); err != 0) {
    return errnoText(argv[0], err);
  }
  posix_spawnattr_t attr;
  if (const int err = posix_spawnattr_init(&attr); err != 0) {
    posix_spawn_file_actions_destroy(&actions);
    return errnoText(argv[0], err);
  }

  // The first setup call to fail; the ones after it do no harm.
  int error = 0;
  const auto keep = [&error](const int result) {
    if (error == 0) {
      error = result;
    }
  };
  keep(posix_spawn_file_actions_addopen(
      &actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0));
  keep(posix_spawn_file_actions_addopen(
      &actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0));
  // Opened here, so that a file it cannot open is not taken for a launcher
  // that cannot run.
  const int stderr_fd = ::open(
      stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
  keep(
      stderr_fd != -1
          ? posix_spawn_file_actions_adddup2(&actions, stderr_fd, STDERR_FILENO)
          : posix_spawn_file_actions_addopen(
                &actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0));
#ifndef __APPLE__
  // A session of its own: emacs shell-mode SIGHUPs the whole session of its
  // pty when openroad exits, and that would close the browser tab.
  keep(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID));
#endif

  std::vector<char*> c_args;
  c_args.reserve(argv.size() + 1);
  for (std::string& arg : argv) {
    c_args.push_back(arg.data());
  }
  c_args.push_back(nullptr);

  // NOLINTNEXTLINE(misc-include-cleaner): <spawn.h> declares it too.
  pid_t pid = 0;
  if (error == 0) {
    error = posix_spawnp(
        &pid, c_args[0], &actions, &attr, c_args.data(), launcherEnviron());
  }
  posix_spawnattr_destroy(&attr);
  posix_spawn_file_actions_destroy(&actions);
  if (stderr_fd != -1) {
    ::close(stderr_fd);
  }
  if (error != 0) {
    return errnoText(argv[0], error);
  }
  return awaitLauncher(pid, argv[0], wait);
}

}  // namespace web
