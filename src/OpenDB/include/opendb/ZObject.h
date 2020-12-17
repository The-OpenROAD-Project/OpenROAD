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

#include "ZComponents.h"
#include "ZException.h"
#include "odb.h"

///
/// This file contains an implmentation of a component based software
/// architecture. This implementation is strictly for C++ interfaces. There is
/// no support for external language bindings. And there is no support for
/// multiple interfaces (QueryInterface: one interface is supported per
/// implementation).
///

namespace odb {

template <class T>
class ZPtr;
class ZContext;
class ZObject;

#define Z_SUCCEEDED(r) (r == Z_OK)
#define Z_FAILED(r) (r != Z_OK)
#define Z_OK 0
#define Z_ERROR_NO_INTERFACE 1
#define Z_ERROR_OUT_OF_MEMORY 2
#define Z_ERROR_NO_COMPONENT 3

///
/// ZINTERFACE_ID -
/// Associate the interface identifier this the interface. This macro
/// should be added to the public section of the interface class.
///
#define ZINTERFACE_ID(INTERFACE) \
  enum                           \
  {                              \
    ZIID = ZIID_##INTERFACE      \
  }

///
/// ZCID - Returns the component interface identifier. You must include
/// ZComponents.h to use this macro.
///
#define ZCID(COMPONENT) (ZCID_##COMPONENT)

///
/// ZIID - Returns the component interface identifier. You must include
/// ZComponents.h to use this macro.
///
#define ZIID(INTERFACE) (ZIID_##INTERFACE)

///
/// adsCreateComponent - Create a new instance of this compomnent, return a
/// pointer to the specified interface.
///
/// Returns: Z_OK, Z_ERROR_NO_COMPONENT, Z_ERROR_NO_INTERFACE
///
/// result is set to the interface pointer on success, otherwise it is set to
/// NULL.
///
int adsCreateComponent(const ZContext& context,
                       ZComponentID    cid,
                       ZInterfaceID    iid,
                       void**          result);

///
/// adsNewComponent - Creates a component specified by the component-identifier.
/// The ZPtr is assigned the specified interface of this component. A run-time
/// error will occur if this component does not support the specified interface.
///
template <class INTERFACE>
inline int adsNewComponent(const ZContext&  context,
                           ZComponentID     cid,
                           ZPtr<INTERFACE>& ptr)
{
  INTERFACE*   p;
  ZInterfaceID iid = (ZInterfaceID) INTERFACE::ZIID;
  int          r   = adsCreateComponent(context, cid, iid, (void**) &p);

  ptr = (INTERFACE*) p;

  if (r == Z_OK)
    p->Release();

  return r;
}

///
/// ZObject - Base class of all interfaces.
///
class ZObject
{
 public:
  typedef ZObject _zobject_traits;
  enum
  {
    ZIID = 0
  };
  ZObject();
  virtual ~ZObject();
  virtual uint AddRef()                                     = 0;
  virtual uint Release()                                    = 0;
  virtual int  QueryInterface(ZInterfaceID iid, void** ref) = 0;
};

///
/// ZPtr - Smart pointer class to access methods of interface.
///
template <class T>
class ZPtr
{
  T* _p;

  void setPtr(T* p)
  {
    if (_p)
      _p->Release();

    _p = p;
  }

  T& operator*();  // Do not allow derefencing.

 public:
  ZPtr() { _p = NULL; }

  ZPtr(ZObject* p)
  {
    if (p == NULL)
      _p = NULL;
    else {
      void* v;
      int   r = p->QueryInterface((ZInterfaceID) T::ZIID, &v);

      if (r != Z_OK) {
        assert(r == Z_OK);  // throw the assert in debug mode
        throw ZException("ZPtr: Invalid interface assignment");
      }

      _p = (T*) v;
    }
  }

  ZPtr(const ZPtr<T>& p)
  {
    _p = p._p;

    if (_p)
      _p->AddRef();
  }

  ~ZPtr()
  {
    if (_p)
      _p->Release();
  }

  ZPtr<T>& operator=(ZObject* p)
  {
    if (p == NULL)
      setPtr(NULL);
    else {
      void* v;
      int   r = p->QueryInterface((ZInterfaceID) T::ZIID, &v);

      if (r != Z_OK) {
        assert(r == Z_OK);  // throw the assert in debug mode
        throw ZException("ZPtr: Invalid interface assignment");
      }

      setPtr((T*) v);
    }

    return *this;
  }

  ZPtr<T>& operator=(ZPtr<T>& p)
  {
    if (this != &p) {
      setPtr(p._p);

      if (_p)
        _p->AddRef();
    }

    return *this;
  }

  bool operator==(const ZPtr<T>& p) const { return _p == p._p; }
  bool operator!=(const ZPtr<T>& p) const { return _p != p._p; }
  bool operator==(const T* p) const { return _p == p; }
  bool operator!=(const T* p) const { return _p != p; }

  // pointer operator
  T* operator->() { return _p; }

  // get the pointer
  T* getPtr() { return _p; }

  // cast operator to a ZObject
  operator ZObject*() const { return (ZObject*) _p; }
};

}  // namespace odb


