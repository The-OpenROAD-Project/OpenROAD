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

#ifndef _FR_VIARULEGEN_H_
#define _FR_VIARULEGEN_H_

#include <memory>
#include "db/infra/frBox.h"

namespace fr {
  class frViaRuleGenerate {
  public:
    // constructors
    frViaRuleGenerate(const frString &nameIn): name(nameIn), isDefault(false), botEnc(), cutRect(), 
                                               cutSpacing(), topEnc(), botLayerNum(0), cutLayerNum(0),
                                               topLayerNum(0) {}
    // getters
    const frString& getName() const {
      return name;
    }
    bool getDefault() const {
      return isDefault;
    }
    const frPoint& getLayer1Enc() const {
      return botEnc;
    }
    const frBox& getCutRect() const {
      return cutRect;
    }
    const frPoint& getCutSpacing() const {
      return cutSpacing;
    }
    const frPoint& getLayer2Enc() const {
      return topEnc;
    }
    frLayerNum getLayer1Num() const {
      return botLayerNum;
    }
    frLayerNum getLayer2Num() const {
      return topLayerNum;
    }
    frLayerNum getCutLayerNum() const {
      return cutLayerNum;
    }
    // setters
    void setDefault(bool in) {
      isDefault = in;
    }
    void setLayer1Enc(const frPoint &in) {
      botEnc = in;
    }
    void setCutRect(const frBox &in) {
      cutRect = in;
    }
    void setCutSpacing(const frPoint &in) {
      cutSpacing = in;
    }
    void setLayer2Enc(const frPoint &in) {
      topEnc = in;
    }
    void setLayer1Num(frLayerNum in) {
      botLayerNum = in;
    }
    void setCutLayerNum(frLayerNum in) {
      cutLayerNum = in;
    }
    void setLayer2Num(frLayerNum in) {
      topLayerNum = in;
    }
  protected:
    frString                                 name;
    bool                                     isDefault;
    frPoint                                  botEnc;
    frBox                                    cutRect;
    frPoint                                  cutSpacing;
    frPoint                                  topEnc;
    frLayerNum                               botLayerNum;
    frLayerNum                               cutLayerNum;
    frLayerNum                               topLayerNum;
  };
}

#endif
