// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <exception>

#include "absl/synchronization/mutex.h"

namespace utl {

// For capturing an exception in an OpenMP worker thread and rethrowing
// it after the pragma is over in the main thread.  Based on
// https://stackoverflow.com/questions/11828539/elegant-exceptionhandling-in-openmp
class ThreadException
{
 public:
  void capture()
  {
    absl::MutexLock guard(&lock_);
    ptr_ = std::current_exception();
  }

  void rethrow()
  {
    if (ptr_) {
      std::rethrow_exception(ptr_);
    }
  }

  bool hasException() const { return ptr_ != nullptr; }

 private:
  std::exception_ptr ptr_;
  absl::Mutex lock_;
};

}  // namespace utl
