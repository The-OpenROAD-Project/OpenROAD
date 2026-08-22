// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Swig uses this to generate Python interface methods that are called from
// "openroad -python".  This exposes the Watermark methods directly rather than
// the commands the Tcl side wraps them in, so a flow written in Python can
// embed a mark and check its own work in the same process.

#ifdef BAZEL
%module(package="src.wmk") wmk
#else
%module wmk_py
#endif

%{
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

#include "HmacSha256.h"
#include "ord/OpenRoad.hh"
#include "wmk/Watermark.h"
%}

%include <std_string.i>
%include "../../Exception-py.i"

// A key is 32 raw bytes, and nothing in Python builds a std::array.  Without a
// conversion every keyed method here would be present but uncallable, which is
// worse than not offering them: the module would look complete and refuse to
// work.  So take what a caller already has -- a bytes object of the right
// length, or the same 64-character hex string the Tcl commands accept.
%typemap(in) const std::array<std::uint8_t, 32>&
    (std::array<std::uint8_t, 32> key_bytes)
{
  if (PyBytes_Check($input)) {
    if (PyBytes_Size($input) != 32) {
      SWIG_exception_fail(
          SWIG_ValueError,
          "a watermark key is 32 bytes; got a bytes object of another length");
    }
    std::memcpy(key_bytes.data(), PyBytes_AsString($input), 32);
  } else if (PyUnicode_Check($input)) {
    // PyUnicode_AsUTF8 is outside the limited API that the Bazel build
    // compiles against, so go through a bytes object, which is inside it.
    PyObject* utf8 = PyUnicode_AsUTF8String($input);
    bool parsed = false;
    if (utf8 != nullptr) {
      parsed = wmk::parse_hex_key32(
          std::string(PyBytes_AsString(utf8), PyBytes_Size(utf8)), key_bytes);
      Py_DECREF(utf8);
    }
    if (!parsed) {
      SWIG_exception_fail(
          SWIG_ValueError,
          "a watermark key given as text must be 64 hex characters");
    }
  } else {
    SWIG_exception_fail(SWIG_TypeError,
                        "expected a watermark key as bytes or a hex string");
  }
  $1 = &key_bytes;
}

// So an overloaded or defaulted argument still dispatches to it.
%typemap(typecheck, precedence = SWIG_TYPECHECK_STRING)
    const std::array<std::uint8_t, 32>&
{
  $1 = (PyBytes_Check($input) || PyUnicode_Check($input)) ? 1 : 0;
}

%include "wmk/Watermark.h"
