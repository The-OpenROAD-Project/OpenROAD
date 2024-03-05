/* Authors: Osama */
/*
 * Copyright (c) 2021, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

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
  void save(T t)
  {
    OutputArchive::save(t);
  }
  frDesign* getDesign() const { return nullptr; }
};

struct frIArchive : InputArchive
{
  frIArchive(std::istream& os, unsigned flags = 0)
      : InputArchive(os, flags), design(nullptr)
  {
  }

  // forward to base class
  template <class T>
  void load(T& t)
  {
    InputArchive::load(t);
  }
  frDesign* getDesign() const { return design; }
  void setDesign(frDesign* in) { design = in; }

 private:
  frDesign* design;
};
}  // namespace drt

// template implementations
#include <boost/archive/impl/archive_serializer_map.ipp>
#include <boost/archive/impl/basic_binary_iarchive.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>
#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>
