// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <istream>
#include <ostream>

#include "boost/archive/binary_iarchive.hpp"
#include "boost/archive/binary_oarchive.hpp"
#include "frDesign.h"
#include "serialization.h"

// using struct to omit a bunch of friend declarations
namespace drt {
struct frOArchive;
struct frIArchive;
using OutputArchive
    = boost::archive::binary_oarchive_impl<frOArchive,
                                           std::ostream::char_type,
                                           std::ostream::traits_type>;
using InputArchive
    = boost::archive::binary_iarchive_impl<frIArchive,
                                           std::istream::char_type,
                                           std::istream::traits_type>;
struct frOArchive : OutputArchive
{
  frOArchive(std::ostream& os, unsigned flags = 0) : OutputArchive(os, flags) {}

  // forward to base class
  template <class T>
  void save(const T& t)
  {
    OutputArchive::save(t);
  }
  frDesign* getDesign() const { return nullptr; }
};

struct frIArchive : InputArchive
{
  frIArchive(std::istream& os, unsigned flags = 0) : InputArchive(os, flags) {}

  // forward to base class
  template <class T>
  void load(T& t)
  {
    InputArchive::load(t);
  }
  frDesign* getDesign() const { return design_; }
  void setDesign(frDesign* in) { design_ = in; }

 private:
  frDesign* design_{nullptr};
};
}  // namespace drt

// template implementations
#include "boost/archive/impl/archive_serializer_map.ipp"
#include "boost/archive/impl/basic_binary_iarchive.ipp"
#include "boost/archive/impl/basic_binary_iprimitive.ipp"
#include "boost/archive/impl/basic_binary_oarchive.ipp"
#include "boost/archive/impl/basic_binary_oprimitive.ipp"
