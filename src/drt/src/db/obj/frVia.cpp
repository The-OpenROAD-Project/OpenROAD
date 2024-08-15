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

#include "db/obj/frVia.h"

#include "db/drObj/drVia.h"
#include "distributed/frArchive.h"
#include "serialization.h"

namespace drt {

frVia::frVia(const drVia& in)
{
  origin_ = in.getOrigin();
  viaDef_ = in.getViaDef();
  setTapered(in.isTapered());
  setBottomConnected(in.isBottomConnected());
  setTopConnected(in.isTopConnected());
  setIsLonely(in.isLonely());
}

template <class Archive>
void frVia::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<frRef>(*this);
  (ar) & origin_;
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
  (ar) & index_in_owner_;
  serializeViaDef(ar, viaDef_);
  serializeBlockObject(ar, owner_);
}

template void frVia::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void frVia::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);

}  // namespace drt
