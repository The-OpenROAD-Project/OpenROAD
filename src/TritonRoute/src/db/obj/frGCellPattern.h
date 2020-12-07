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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FR_GCELLPATTERN_H_
#define _FR_GCELLPATTERN_H_

#include "db/obj/frBlockObject.h"

namespace fr {
  class frGCellPattern: public frBlockObject {
  public:
    // constructors
    frGCellPattern(): horizontal(false), startCoord(0), spacing(0), count(0) {}
    // getters
    bool isHorizontal() const {
      return horizontal;
    }
    frCoord getStartCoord() const {
      return startCoord;
    }
    frUInt4 getSpacing() const {
      return spacing;
    }
    frUInt4 getCount() const {
      return count;
    }
    // setters
    void setHorizontal(bool isH) {
      horizontal = isH;
    }
    void setStartCoord(frCoord scIn) {
      startCoord = scIn;
    }
    void setSpacing(frUInt4 sIn) {
      spacing = sIn;
    }
    void setCount(frUInt4 cIn) {
      count = cIn;
    }
    // others
    frBlockObjectEnum typeId() const override { return frcGCellPattern;}
  protected:
    bool    horizontal;
    frCoord startCoord;
    frUInt4 spacing;
    frUInt4 count;
  };
}

#endif
