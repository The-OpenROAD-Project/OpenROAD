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

#include <algorithm>
#include "dbSet.h"

namespace odb {
//
// diff_object - Diff the object if this is a deep-diff, otherwise diff the
// field.
//
// diff_object - Diff the object if this is a deep-diff, otherwise diff the
template <class T>
inline void diff_object(dbDiff&     diff,
                        const char* field,
                        dbId<T>     lhs,
                        dbId<T>     rhs,
                        dbTable<T>* lhs_tbl,
                        dbTable<T>* rhs_tbl)
{
  if (diff.deepDiff() == false) {
    diff.diff(field, (unsigned int) lhs, (unsigned int) rhs);
  } else {
    if (lhs != 0 && rhs != 0) {
      T* o1 = lhs_tbl->getPtr(lhs);
      T* o2 = rhs_tbl->getPtr(rhs);
      o1->differences(diff, field, *o2);
    } else if (lhs != 0) {
      T* o1 = lhs_tbl->getPtr(lhs);
      o1->out(diff, dbDiff::LEFT, field);
    } else if (rhs != 0) {
      T* o2 = rhs_tbl->getPtr(rhs);
      o2->out(diff, dbDiff::RIGHT, field);
    }
  }
}

template <class T>
inline void diff_object(dbDiff&          diff,
                        const char*      field,
                        dbId<T>          lhs,
                        dbId<T>          rhs,
                        dbArrayTable<T>* lhs_tbl,
                        dbArrayTable<T>* rhs_tbl)
{
  if (diff.deepDiff() == false) {
    diff.diff(field, (unsigned int) lhs, (unsigned int) rhs);
  } else {
    if (lhs != 0 && rhs != 0) {
      T* o1 = lhs_tbl->getPtr(lhs);
      T* o2 = rhs_tbl->getPtr(rhs);
      o1->differences(diff, field, *o2);
    } else if (lhs != 0) {
      T* o1 = lhs_tbl->getPtr(lhs);
      o1->out(diff, dbDiff::LEFT, field);
    } else if (rhs != 0) {
      T* o2 = rhs_tbl->getPtr(rhs);
      o2->out(diff, dbDiff::RIGHT, field);
    }
  }
}

template <class T>
inline void diff_out_object(dbDiff&     diff,
                            char        side,
                            const char* field,
                            dbId<T>     id,
                            dbTable<T>* tbl)
{
  if (diff.deepDiff() == false)
    diff.out(side, field, (unsigned int) id);

  else if (id != 0) {
    T* o = tbl->getPtr(id);
    o->out(diff, side, field);
  }
}

template <class T>
inline void diff_out_object(dbDiff&          diff,
                            char             side,
                            const char*      field,
                            dbId<T>          id,
                            dbArrayTable<T>* tbl)
{
  if (diff.deepDiff() == false)
    diff.out(side, field, (unsigned int) id);

  else if (id != 0) {
    T* o = tbl->getPtr(id);
    o->out(diff, side, field);
  }
}

//
// diff_set - This function will diff the "set" this field represents if
//            this is a "deep" differences. Otherwise, only the field is
//            diff'ed.
//
template <class T>
inline void diff_set(dbDiff&     diff,
                     const char* field,
                     dbId<T>     lhs,
                     dbId<T>     rhs,
                     dbObject*   lhs_owner,
                     dbObject*   rhs_owner,
                     dbIterator* lhs_itr,
                     dbIterator* rhs_itr)
{
  if (diff.deepDiff() == false) {
    diff.diff(field, (unsigned int) lhs, (unsigned int) rhs);
  } else {
    typename dbSet<T>::iterator itr;

    dbSet<T>        lhs_set(lhs_owner, lhs_itr);
    std::vector<T*> lhs_vec;

    for (itr = lhs_set.begin(); itr != lhs_set.end(); ++itr)
      lhs_vec.push_back(*itr);

    dbSet<T>        rhs_set(rhs_owner, rhs_itr);
    std::vector<T*> rhs_vec;

    for (itr = rhs_set.begin(); itr != rhs_set.end(); ++itr)
      rhs_vec.push_back(*itr);
#ifndef WIN32
    set_symmetric_diff(diff, field, lhs_vec, rhs_vec);
#endif
  }
}

template <class T>
inline void diff_out_set(dbDiff&     diff,
                         char        side,
                         const char* field,
                         dbId<T>     id,
                         dbObject*   owner,
                         dbIterator* set_itr)
{
  if (diff.deepDiff() == false) {
    diff.out(side, field, (unsigned int) id);
  } else {
    typename dbSet<T>::iterator itr;

    diff.begin_object("<> %s\n", field);
    dbSet<T> oset(owner, set_itr);

    for (itr = oset.begin(); itr != oset.end(); ++itr)
      (*itr)->out(diff, side, NULL);

    diff.end_object();
  }
}

//
// set_symmetric_diff - Diff the two tables
//
template <class T>
inline void set_symmetric_diff(dbDiff&     diff,
                               const char* field,
                               dbTable<T>& lhs,
                               dbTable<T>& rhs)
{
  std::vector<T*> lhs_vec;
  std::vector<T*> rhs_vec;
  lhs.getObjects(lhs_vec);
  rhs.getObjects(rhs_vec);
#ifndef WIN32
  set_symmetric_diff(diff, field, lhs_vec, rhs_vec);
#endif
}

//
// set_symmetric_diff - Diff the two tables
//
template <class T>
inline void set_symmetric_diff(dbDiff&          diff,
                               const char*      field,
                               dbArrayTable<T>& lhs,
                               dbArrayTable<T>& rhs)
{
  std::vector<T*> lhs_vec;
  std::vector<T*> rhs_vec;
  lhs.getObjects(lhs_vec);
  rhs.getObjects(rhs_vec);
#ifndef WIN32
  set_symmetric_diff(diff, field, lhs_vec, rhs_vec);
#endif
}

//
// set_symmetric_diff - Diff the two vectors of objects. The objects must have
// the "operator<" defined and a the equal method defined.
//
template <class T>
inline void set_symmetric_diff(dbDiff&          diff,
                               const char*      field,
                               std::vector<T*>& lhs,
                               std::vector<T*>& rhs)
{
  diff.begin_object("<> %s\n", field);
  std::sort(lhs.begin(), lhs.end(), dbDiffCmp<T>());
  std::sort(rhs.begin(), rhs.end(), dbDiffCmp<T>());

  typename std::vector<T*>::iterator end;
  std::vector<T*>                    symmetric_diff;

  symmetric_diff.resize(lhs.size() + rhs.size());

  end = std::set_symmetric_difference(lhs.begin(),
                                      lhs.end(),
                                      rhs.begin(),
                                      rhs.end(),
                                      symmetric_diff.begin(),
                                      dbDiffCmp<T>());

  typename std::vector<T*>::iterator i1 = lhs.begin();
  typename std::vector<T*>::iterator i2 = rhs.begin();
  typename std::vector<T*>::iterator sd = symmetric_diff.begin();

  while ((i1 != lhs.end()) && (i2 != rhs.end())) {
    T* o1 = *i1;
    T* o2 = *i2;

    if (o1 == *sd) {
      o1->out(diff, dbDiff::LEFT, NULL);
      ++i1;
      ++sd;
    } else if (o2 == *sd) {
      o2->out(diff, dbDiff::RIGHT, NULL);
      ++i2;
      ++sd;
    } else  // equal keys
    {
      // compare internals
      o1->differences(diff, NULL, *o2);
      ++i1;
      ++i2;
    }
  }

  for (; i1 != lhs.end(); ++i1) {
    T* o1 = *i1;
    o1->out(diff, dbDiff::LEFT, NULL);
  }

  for (; i2 != rhs.end(); ++i2) {
    T* o2 = *i2;
    o2->out(diff, dbDiff::RIGHT, NULL);
  }

  diff.end_object();
}

//
// set_symmetric_diff - Diff the two vectors of objects. The objects must have
// the "operator<" defined.
//
template <class T>
inline void set_symmetric_diff(dbDiff&               diff,
                               const char*           field,
                               const std::vector<T>& lhs_v,
                               const std::vector<T>& rhs_v)
{
  std::vector<T> lhs = lhs_v;
  std::vector<T> rhs = rhs_v;
  diff.begin_object("<> %s\n", field);
  std::sort(lhs.begin(), lhs.end());
  std::sort(rhs.begin(), rhs.end());

  typename std::vector<T>::iterator end;
  std::vector<T>                    symmetric_diff;

  symmetric_diff.resize(lhs.size() + rhs.size());

  end = std::set_symmetric_difference(
      lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), symmetric_diff.begin());

  typename std::vector<T>::iterator i1 = lhs.begin();
  typename std::vector<T>::iterator i2 = rhs.begin();
  typename std::vector<T>::iterator sd = symmetric_diff.begin();

  while ((i1 != lhs.end()) && (i2 != rhs.end())) {
    T& o1 = *i1;
    T& o2 = *i2;

    if (o1 == *sd) {
      diff.out(dbDiff::LEFT, "", o1);
      ++i1;
      ++sd;
    } else if (o2 == *sd) {
      diff.out(dbDiff::RIGHT, "", o2);
      ++i2;
      ++sd;
    } else  // equal keys
    {
      ++i1;
      ++i2;
    }
  }

  for (; i1 != lhs.end(); ++i1) {
    T& o1 = *i1;
    diff.out(dbDiff::LEFT, "", o1);
  }

  for (; i2 != rhs.end(); ++i2) {
    T& o2 = *i2;
    diff.out(dbDiff::RIGHT, "", o2);
  }

  diff.end_object();
}

}  // namespace odb


