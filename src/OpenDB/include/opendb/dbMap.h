///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <map>
#include <vector>

#include "odb.h"
#include "dbSet.h"

namespace odb {

///
/// This class creates a assocation between
/// the objects of dbSet<T> to a an object of type "D".
/// If "D" is a class object, then "D" must have a public default constructor
/// and a public destructor.
///
/// NOTE: The map becomes invalid if the dbSet membership changes, i.e.,
/// a member is added or deleted from the set.
///
/// If the map becomes invalid, access operations may fail.
///
template <class T, class D>
class dbMap
{
  dbSet<T>         _set;
  std::map<T*, D>* _map;     // map used if set is not sequential
  std::vector<D>*  _vector;  // vector used if set is sequential

  // Map cannot be assigned or copied!
  dbMap(const dbMap&);
  dbMap& operator=(const dbMap&);

 public:
  ///
  /// Create a new map from the set. The data-objects are initialized
  /// to the default constructor of the object type.
  ///
  dbMap(const dbSet<T>& set);

  ///
  /// Destructor.
  ///
  ~dbMap();

  ///
  /// D & operator[T *] const
  /// Access to a data object of key "T *".
  ///
  /// Example:
  ///
  /// dbSet<dbNet> nets = block->getNets();
  /// dbMap<dbNet, int> net_weight;
  /// dbSet<dbNet>::iterator itr;
  ///
  /// for( itr = nets.begin(); itr != nets.end(); ++itr )
  /// {
  ///     dbNet * net = *itr;
  ///     net_weight[net] = 1;
  /// }
  ///
  D& operator[](T* object);

  ///
  /// const D & operator[T *] const
  /// Const access to a data object of key "T *".
  ///
  /// Example:
  ///
  /// dbSet<dbNet> nets = block->getNets();
  /// dbMap<dbNet, int> net_weight;
  /// dbSet<dbNet>::iterator itr;
  ///
  /// for( itr = nets.begin(); itr != nets.end(); ++itr )
  /// {
  ///     dbNet * net = *itr;
  ///     int weight = net_weight[net];
  ///     printf("net(%d) weight = %d\n", net->getId(), weight );
  /// }
  ///
  const D& operator[](T* object) const;
};

}  // namespace odb

#include "dbMap.hpp"


