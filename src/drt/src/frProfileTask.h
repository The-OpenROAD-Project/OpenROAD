// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#ifdef HAS_VTUNE
#include <ittnotify.h>
#endif

namespace drt {

#ifdef HAS_VTUNE
// This class make a VTune task in its scope (RAII).  This is useful
// in VTune to see where the runtime is going with more domain specific
// display.
class ProfileTask
{
 public:
  ProfileTask(const char* name) : done_(false)
  {
    domain_ = __itt_domain_create("TritonRoute");
    name_ = __itt_string_handle_create(name);
    __itt_task_begin(domain_, __itt_null, __itt_null, name_);
  }

  ~ProfileTask()
  {
    if (!done_) {
      __itt_task_end(domain_);
    }
  }

  // Useful if you don't want to have to introduce a scope
  // just to note a task.
  void done()
  {
    done_ = true;
    __itt_task_end(domain_);
  }

 private:
  __itt_domain* domain_;
  __itt_string_handle* name_;
  bool done_;
};

#else

// No-op version
class ProfileTask
{
 public:
  ProfileTask(const char* name) {}
  void done() {}
};
#endif

}  // namespace drt
