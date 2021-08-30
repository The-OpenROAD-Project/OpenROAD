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

#ifndef _GR_FIG_H_
#define _GR_FIG_H_

#include <memory>

#include "db/grObj/grBlockObject.h"
#include "db/infra/frBox.h"
#include "db/infra/frTransform.h"

namespace fr {
class grFig : public grBlockObject
{
 public:
  // constructors
  grFig() : grBlockObject() {}
  // getters
  virtual void getBBox(frBox& box) const = 0;
  // setters
  // others
 protected:
};

class frNet;
class grNet;
class frNode;
class grNode;
class grConnFig : public grFig
{
 public:
  // constructors
  grConnFig() : grFig() {}
  // getters
  virtual bool hasNet() const = 0;
  virtual frNet* getNet() const = 0;
  virtual bool hasGrNet() const = 0;
  virtual grNet* getGrNet() const = 0;
  virtual frNode* getChild() const = 0;
  virtual frNode* getParent() const = 0;
  virtual grNode* getGrChild() const = 0;
  virtual grNode* getGrParent() const = 0;
  // setters
  virtual void addToNet(frBlockObject* in) = 0;
  virtual void removeFromNet() = 0;
  virtual void setChild(frBlockObject* in) = 0;
  virtual void setParent(frBlockObject* in) = 0;
  // others

  /* from frFig
   * getBBox
   * move
   * overlaps
   */
 protected:
};

class grPin;
class grPinFig : public grConnFig
{
 public:
  grPinFig() : grConnFig() {}
  // getters
  virtual bool hasPin() const = 0;
  virtual grPin* getPin() const = 0;
  // setters
  virtual void addToPin(grPin* in) = 0;
  virtual void removeFromPin() = 0;
  // others

  /* from grConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */

  /* from grFig
   * getBBox
   * move
   * overlaps
   */
 protected:
};
}  // namespace fr

#endif
