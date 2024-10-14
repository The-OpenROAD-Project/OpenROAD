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
  fetchFromGitHub,
  libsForQt5,
  boost183,
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
  re2,
  swig4,
  pkg-config,
  cmake,
  gnumake,
  flex,
  bison,
  clang-tools_14,
  gtest,
  # or-tools
  stdenv,
  overrideSDK,
  git,
}: let
  or-tools' =
    (or-tools.override {
      ## Alligned alloc not available on the default SDK for x86_64-darwin (10.12!!)
      stdenv =
        if stdenv.isDarwin
        then (overrideSDK stdenv "11.0")
        else stdenv;
    })
    .overrideAttrs (finalAttrs: previousAttrs: {
      # Based on https://github.com/google/or-tools/commit/af44f98dbeb905656b5a9fc664b5fdcffcbe1f60
      # Stops CMake going haywire on reconfigures
      postPatch =
        previousAttrs.postPatch
        + ''
          sed -Ei.bak 's/(NOT\s+\w+_FOUND\s+AND\s+)+//' cmake/ortoolsConfig.cmake.in
          sed -Ei.bak 's/NOT absl_FOUND/NOT TARGET absl::base/' cmake/ortoolsConfig.cmake.in
        '';
    });
  self = clangStdenv.mkDerivation (finalAttrs: {
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
    '';
    
    qt5Libs = [
      libsForQt5.qt5.qtbase
      libsForQt5.qt5.qtcharts
      libsForQt5.qt5.qtsvg
      libsForQt5.qt5.qtdeclarative
    ];
    
    QT_PLUGIN_PATH = lib.makeSearchPathOutput "bin" "lib/qt-${libsForQt5.qt5.qtbase.version}/plugins" self.qt5Libs;

    buildInputs = self.qt5Libs ++ [
      boost183
      eigen
      cudd
      tcl
      python3
      readline
      tclreadline
      spdlog
      libffi
      llvmPackages.openmp

      lemon-graph
      or-tools'
      glpk
      zlib
      clp
      cbc
      re2
      gtest
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
      clang-tools_14
    ];
  });
in
  self
