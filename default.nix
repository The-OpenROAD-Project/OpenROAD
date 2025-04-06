# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023-2025, The OpenROAD Authors

{
  flake,
  lib,
  clangStdenv,
  fetchFromGitHub,
  fetchzip,
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
  fetchedGtest = fetchzip {
    url = "https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip";
    sha256 = "sha256-t0RchAHTJbuI5YW4uyBPykTvcjy90JW9AOPNjIhwh6U=";
  };
  self = clangStdenv.mkDerivation (finalAttrs: {
    name = "openroad";

    src = flake;

    cmakeFlags = [
      "-DTCL_LIBRARY=${tcl}/lib/libtcl${clangStdenv.hostPlatform.extensions.sharedLibrary}"
      "-DTCL_HEADER=${tcl}/include/tcl.h"
      "-DUSE_SYSTEM_BOOST:BOOL=ON"
      "-DVERBOSE=1"
      "-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${fetchedGtest}"
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
