// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#define PY_SSIZE_T_CLEAN
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Python.h"

extern "C" {
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern PyObject* PyInit__odb_py();
}

int main(int argc, char* argv[])
{
  if (PyImport_AppendInittab("_odb_py", PyInit__odb_py) == -1) {
    fprintf(stderr, "Error: could not extend in-built modules table\n");
    exit(1);
  }
  wchar_t** args = new wchar_t*[argc];
  for (size_t i = 0; i < argc; i++) {
    size_t sz = strlen(argv[i]);
    args[i] = new wchar_t[sz + 1];
    args[i][sz] = '\0';
    for (size_t j = 0; j < sz; j++) {
      args[i][j] = (wchar_t) argv[i][j];
    }
  }

  Py_Initialize();
  Py_Main(argc, args);
}
