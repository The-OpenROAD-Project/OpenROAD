// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

namespace utl {

// RAII mechanism to facilitate using operation-specific settings.
template <typename T>
class SetAndRestore
{
 public:
  SetAndRestore(T& storage, const T& new_value)
      : storage_(storage), old_value_(storage)
  {
    storage_ = new_value;
  }

  ~SetAndRestore() { storage_ = old_value_; }

 private:
  T& storage_;
  T old_value_;
};

}  // namespace utl
