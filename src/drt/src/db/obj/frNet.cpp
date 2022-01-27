/* Authors: Osama */
/*
 * Copyright (c) 2022, The Regents of the University of California
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

#include "db/obj/frNet.h"

#include "distributed/frArchive.h"
#include "frInstTerm.h"

using namespace fr;
template <class Archive>
void frNet::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<frBlockObject>(*this);
  (ar) & name_;
  (ar) & instTerms_;
  (ar) & bterms_;
  (ar) & shapes_;
  (ar) & vias_;
  (ar) & pwires_;
  // TODO for gr support
  // (ar) & grShapes_;
  // (ar) & grVias_;
  (ar) & nodes_;
  (ar) & root_;
  (ar) & rootGCellNode_;
  (ar) & firstNonRPinNode_;
  (ar) & rpins_;
  (ar) & guides_;
  (ar) & type_;
  (ar) & modified_;
  (ar) & isFakeNet_;
  (ar) & ndr_;
  (ar) & absPriorityLvl;
  (ar) & isClock_;
  (ar) & isSpecial_;

  // The list members can container an iterator representing their position
  // in the list for fast removal.  It is tricky to serialize the iterator
  // so just reset them from the list after loading.
  if (is_loading(ar)) {
    for (auto it = shapes_.begin(); it != shapes_.end(); ++it) {
      (*it)->setIter(it);
    }
    for (auto it = vias_.begin(); it != vias_.end(); ++it) {
      (*it)->setIter(it);
    }
    for (auto it = pwires_.begin(); it != pwires_.end(); ++it) {
      (*it)->setIter(it);
    }
    for (auto it = grShapes_.begin(); it != grShapes_.end(); ++it) {
      (*it)->setIter(it);
    }
    for (auto it = grVias_.begin(); it != grVias_.end(); ++it) {
      (*it)->setIter(it);
    }
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
      (*it)->setIter(it);
    }
    for (auto term : instTerms_)
      term->addToNet(this);
    for (auto term : bterms_)
      term->addToNet(this);
  }
}

// Explicit instantiations
template void frNet::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void frNet::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);