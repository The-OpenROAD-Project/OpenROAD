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

#ifndef _FR_SEGSTYLE_H_
#define _FR_SEGSTYLE_H_

#include "frBaseTypes.h"

namespace fr {
  class frEndStyle {
  public:
    // constructor
    frEndStyle(): style(frcExtendEndStyle) {}
    frEndStyle(const frEndStyle &styleIn): style(styleIn.style) {}
    frEndStyle(frEndStyleEnum styleIn): style(styleIn) {}
    // setters
    void set(frEndStyleEnum styleIn) {
      style = styleIn;
    }
    void set(const frEndStyle &styleIn) {
      style = styleIn.style;
    }
    // getters
    operator frEndStyleEnum() const {
      return style;
    }
  protected:
    frEndStyleEnum style;
  };

  class frSegStyle {
  public:
    // constructor
    frSegStyle(): beginExt(0), endExt(0), width(0), beginStyle(), endStyle() {}
    frSegStyle(const frSegStyle &in): beginExt(in.beginExt), endExt(in.endExt),
                                      width(in.width), beginStyle(in.beginStyle),
                                      endStyle(in.endStyle) {}
    // setters
    void setWidth(frUInt4 widthIn) {
      width = widthIn;
    }
    void setBeginStyle(const frEndStyle &style, frUInt4 ext = 0) {
      beginStyle.set(style);
      beginExt = ext;
    }
    void setEndStyle(const frEndStyle &style, frUInt4 ext = 0) {
      endStyle.set(style);
      endExt = ext;
    }
    void setBeginExt(const frUInt4 &in) {
      beginExt = in;
    }
    void setEndExt(const frUInt4 &in) {
      endExt = in;
    }
    // getters
    frUInt4 getWidth() const {
      return width;
    }
    frUInt4 getBeginExt() const {
      return beginExt;
    }
    frEndStyle getBeginStyle() const {
      return beginStyle;
    }
    frUInt4 getEndExt() const {
      return endExt;
    }
    frEndStyle getEndStyle() const {
      return endStyle;
    }
  protected:
    frUInt4 beginExt;
    frUInt4 endExt;
    frUInt4 width;
    frEndStyle beginStyle;
    frEndStyle endStyle;
  };

}

#endif
