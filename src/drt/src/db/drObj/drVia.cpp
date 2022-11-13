/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
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

#include "db/drObj/drVia.h"

#include "db/drObj/drNet.h"
#include "db/obj/frVia.h"
#include "distributed/frArchive.h"

using namespace std;
using namespace fr;

drVia::drVia(const frVia& in)
    : viaDef_(in.getViaDef()), owner_(nullptr), beginMazeIdx_(), endMazeIdx_()
{
  origin_ = in.getOrigin();
  setTapered(in.isTapered());
  setBottomConnected(in.isBottomConnected());
  setTopConnected(in.isTopConnected());
}

template <class Archive>
void drVia::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<drRef>(*this);
  (ar) & origin_;
  (ar) & owner_;
  (ar) & beginMazeIdx_;
  (ar) & endMazeIdx_;
  serializeViaDef(ar, viaDef_);
  bool tmp = false;
  if (is_loading(ar)) {
    (ar) & tmp;
    tapered_ = tmp;
    (ar) & tmp;
    bottomConnected_ = tmp;
    (ar) & tmp;
    topConnected_ = tmp;

  } else {
    tmp = tapered_;
    (ar) & tmp;
    tmp = bottomConnected_;
    (ar) & tmp;
    tmp = topConnected_;
    (ar) & tmp;
  }
}

// Explicit instantiations
template void drVia::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void drVia::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);
