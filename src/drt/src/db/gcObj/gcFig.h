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

#ifndef _GC_FIG_H_
#define _GC_FIG_H_

#include <memory>

#include "db/gcObj/gcBlockObject.h"
#include "db/infra/frBox.h"
#include "db/infra/frTransform.h"

namespace fr {
class gcFig : public gcBlockObject
{
 public:
  // getters
  // setters
  // others
 protected:
  // constructors
  gcFig() : gcBlockObject() {}
  gcFig(const gcFig& in) : gcBlockObject(in) {}

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gcBlockObject>(*this);
  }

  friend class boost::serialization::access;
};

class gcNet;
class gcConnFig : public gcFig
{
 public:
  // getters
  virtual bool hasNet() const = 0;
  virtual gcNet* getNet() const = 0;
  // setters
  virtual void addToNet(gcNet* in) = 0;
  virtual void removeFromNet() = 0;
  // others

  /* from gcFig
   * getBBox
   * move
   * overlaps
   */
 protected:
  // constructors
  gcConnFig() : gcFig() {}
  gcConnFig(const gcConnFig& in) : gcFig(in) {}

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gcFig>(*this);
  }

  friend class boost::serialization::access;
};

class gcPin;
class gcPinFig : public gcConnFig
{
 public:
  // getters
  virtual bool hasPin() const = 0;
  virtual gcPin* getPin() const = 0;
  // setters
  virtual void addToPin(gcPin* in) = 0;
  virtual void removeFromPin() = 0;
  // others

  /* from gcConnFig
   * hasNet
   * getNet
   * addToNet
   * removedromNet
   */

  /* from gcFig
   * getBBox
   * move
   * overlaps
   */
 protected:
  gcPinFig() : gcConnFig() {}
  gcPinFig(const gcPinFig& in) : gcConnFig(in) {}

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gcConnFig>(*this);
  }

  friend class boost::serialization::access;
};

}  // namespace fr

#endif
