// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for sv-elab
//

#include "ir.h"

#include <cstdint>

#include "slang_frontend.h"

namespace ir {

Net::Net(slang_frontend::Trit t) : raw_(syn::Net::undef())
{
  switch (t) {
    case slang_frontend::S0:
      raw_ = syn::Net::zero();
      break;
    case slang_frontend::S1:
      raw_ = syn::Net::one();
      break;
    default:
      raw_ = syn::Net::undef();
      break;
  }
}

slang_frontend::Trit Net::as_trit() const
{
  if (raw_ == syn::Net::zero()) {
    return slang_frontend::S0;
  }
  if (raw_ == syn::Net::one()) {
    return slang_frontend::S1;
  }
  return slang_frontend::Sx;
}

Value::Value(const slang_frontend::TritVector& c)
{
  nets_.reserve(c.size());
  for (auto i : c) {
    nets_.push_back(Net(i).raw());
  }
}

slang_frontend::TritVector Value::as_const() const
{
  std::vector<slang_frontend::Trit> bits;
  bits.reserve(nets_.size());
  for (auto& n : nets_) {
    bits.push_back(Net(n).as_trit());
  }
  return slang_frontend::TritVector(bits);
}

Value Net::repeat(uint64_t width)
{
  return Value(*this).repeat(width);
}

}  // namespace ir
