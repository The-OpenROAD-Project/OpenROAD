###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2023-2024, Efabless Corporation
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################
{
  flake,
  lib,
  clangStdenv,
  clang-tools,
  fetchFromGitHub,
  libsForQt5,
  bzip2,
  boost186,
  eigen,
  cudd,
  ninja,
  tcl,
  python3,
  readline,
  tclreadline,
  spdlog,
  libffi,
  llvmPackages,
  lemon-graph,
  or-tools,
  glpk,
  zlib,
  clp,
  cbc,
  swig4,
  pkg-config,
  cmake,
  gnumake,
  flex,
  bison,
  gtest,
  git,
  or-tools_9_11,
  highs, # for or-tools
  re2, # for or-tools
}: clangStdenv.mkDerivation (finalAttrs: {
    name = "openroad";

    src = flake;

    cmakeFlags = [
      "-DTCL_LIBRARY=${tcl}/lib/libtcl${clangStdenv.hostPlatform.extensions.sharedLibrary}"
      "-DTCL_HEADER=${tcl}/include/tcl.h"
      "-DUSE_SYSTEM_BOOST:BOOL=ON"
      "-DVERBOSE=1"
    ];

    postPatch = ''
      patchShebangs .
    '';

    shellHook = ''
      alias ord-format-changed="${git}/bin/git diff --name-only | grep -E '\.(cpp|cc|c|h|hh)$' | xargs clang-format -i -style=file:.clang-format";
      alias ord-cmake-debug="cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g" -G Ninja $cmakeFlags .."
      alias ord-cmake-release="cmake -DCMAKE_BUILD_TYPE=Release -G Ninja $cmakeFlags .."
    '';

    qt5Libs = [
      libsForQt5.qt5.qtbase
      libsForQt5.qt5.qtcharts
      libsForQt5.qt5.qtsvg
      libsForQt5.qt5.qtdeclarative
    ];

    QT_PLUGIN_PATH = lib.makeSearchPathOutput "bin" "lib/qt-${libsForQt5.qt5.qtbase.version}/plugins" finalAttrs.qt5Libs;

    buildInputs =
      finalAttrs.qt5Libs
      ++ [
        boost186
        cbc
        clp
        cudd
        eigen
        glpk
        gtest
        lemon-graph
        libffi
        llvmPackages.openmp
        python3
        readline
        spdlog
        tcl
        tclreadline
        zlib

        or-tools_9_11
        bzip2
      ];

    nativeBuildInputs = [
      swig4
      pkg-config
      cmake
      ninja
      gnumake
      flex
      bison
      libsForQt5.wrapQtAppsHook
      clang-tools
    ];
  })
