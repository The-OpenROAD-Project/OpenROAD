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

#ifndef _FR_ORIENT_H_
#define _FR_ORIENT_H_

#include "frBaseTypes.h"

namespace fr {
  class frOrient {
  public:
    // constructor
    frOrient(): orient(frcR0) {}
    frOrient(const frOrient &tmpOrient): orient(tmpOrient.orient) {}
    frOrient(frOrientEnum tmpOrient): orient(tmpOrient) {}
    frOrient(const frString &name) {
      if (name == frString("frcR90")) {
        orient = frcR90;
      } else if (name == frString("frcR180")) {
        orient = frcR180;
      } else if (name == frString("frcR270")) {
        orient = frcR270;
      } else if (name == frString("frcMY")) {
        orient = frcMY;
      } else if (name == frString("frcMYR90")) {
        orient = frcMYR90;
      } else if (name == frString("frcMX")) {
        orient = frcMX;
      } else if (name == frString("frcMXR90")) {
        orient = frcMXR90;
      } else {
        orient = frcR0;
      }
    }
    // setters
    void set(frOrientEnum tmpOrient) {
      orient = tmpOrient;
    }
    void set(const frOrient &tmpOrient) {
      orient = tmpOrient.orient;
    }
    void set(const frString &name) {
      if (name == frString("frcR90")) {
        orient = frcR90;
      } else if (name == frString("frcR180")) {
        orient = frcR180;
      } else if (name == frString("frcR270")) {
        orient = frcR270;
      } else if (name == frString("frcMY")) {
        orient = frcMY;
      } else if (name == frString("frcMYR90")) {
        orient = frcMYR90;
      } else if (name == frString("frcMX")) {
        orient = frcMX;
      } else if (name == frString("frcMXR90")) {
        orient = frcMXR90;
      } else {
        orient = frcR0;
      }
    }
    // getters
    //frOrientEnum get() const {
    //  return orient;
    //}
    frString getName() const {
      switch(orient) {
        //case frcR0    : return frString("frcR0");
        case frcR90   : return frString("frcR90");
        case frcR180  : return frString("frcR180");
        case frcR270  : return frString("frcR270");
        case frcMY    : return frString("frcMY");
        case frcMYR90 : return frString("frcMYR90");
        case frcMX    : return frString("frcMX");
        case frcMXR90 : return frString("frcMXR90");
        default       : return frString("frcR0");
      }
    }
    void getName(frString &name) const {
      switch(orient) {
        //case frcR0    : return frString("frcR0");
        case frcR90   : 
          name = "frcR90";
          break;
        case frcR180  : 
          name = "frcR180";
          break;
        case frcR270  : 
          name = "frcR270";
          break;
        case frcMY    : 
          name = "frcMY";
          break;
        case frcMYR90 : 
          name = "frcMYR90";
          break;
        case frcMX    : 
          name = "frcMX";
          break;
        case frcMXR90 : 
          name = "frcMXR90";
          break;
        default       : 
          name = "frcR0";
      }
    }
    // overloads
    //frOrientEnum operator()() const {
    //  return orient;
    //}
    operator frOrientEnum() const {
      return orient;
    }
  protected:
    frOrientEnum orient;
  };
}

#endif
