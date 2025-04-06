// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <functional>
#include <memory>

namespace utl {

template <typename T>
using UniquePtrWithDeleter = std::unique_ptr<T, std::function<void(T*)>>;

}
