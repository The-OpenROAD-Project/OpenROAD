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
# ---
# Original license follows:
#
# Copyright (c) 2003-2024 Eelco Dolstra and the Nixpkgs/NixOS contributors
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
{
  abseil-cpp,
  bzip2,
  cbc,
  cmake,
  DarwinTools, # sw_vers
  eigen,
  ensureNewerSourcesForZipFilesHook,
  fetchFromGitHub,
  glpk,
  lib,
  pkg-config,
  protobuf,
  re2,
  llvmPackages_17,
  swig,
  unzip,
  zlib,
  highs,
}: let
  stdenv = llvmPackages_17.stdenv;
in
  stdenv.mkDerivation (finalAttrs: {
    pname = "or-tools";
    version = "9.11";

    src = fetchFromGitHub {
      owner = "google";
      repo = "or-tools";
      rev = "v${finalAttrs.version}";
      hash = "sha256-aRhUAs9Otvra7VPJvrf0fhDCGpYhOw1//BC4dFJ7/xI=";
    };

    cmakeFlags =
      [
        "-DBUILD_DEPS=OFF"
        "-DBUILD_absl=OFF"
        "-DCMAKE_INSTALL_BINDIR=bin"
        "-DCMAKE_INSTALL_INCLUDEDIR=include"
        "-DCMAKE_INSTALL_LIBDIR=lib"
        "-DUSE_GLPK=ON"
        "-DUSE_SCIP=OFF"
        "-DPROTOC_PRG=${protobuf}/bin/protoc"
      ]
      ++ lib.optionals stdenv.hostPlatform.isDarwin ["-DCMAKE_MACOSX_RPATH=OFF"];

    strictDeps = true;

    nativeBuildInputs =
      [
        cmake
        ensureNewerSourcesForZipFilesHook
        pkg-config
        swig
        unzip
      ]
      ++ lib.optionals stdenv.hostPlatform.isDarwin [
        DarwinTools
      ];

    buildInputs = [
      abseil-cpp
      bzip2
      cbc
      eigen
      glpk
      re2
      zlib
      highs
    ];

    propagatedBuildInputs = [
      abseil-cpp
      protobuf
    ];

    env.NIX_CFLAGS_COMPILE = toString [
      # fatal error: 'python/google/protobuf/proto_api.h' file not found
      "-I${protobuf.src}"
    ];

    # some tests fail on linux and hang on darwin
    doCheck = false;

    preCheck = ''
      export LD_LIBRARY_PATH=$LD_LIBRARY_PATH''${LD_LIBRARY_PATH:+:}$PWD/lib
    '';

    # This extra configure step prevents the installer from littering
    # $out/bin with sample programs that only really function as tests,
    # and disables the upstream installation of a zipped Python egg that
    # can’t be imported with our Python setup.
    installPhase = ''
      cmake . -DBUILD_EXAMPLES=OFF -DBUILD_PYTHON=OFF -DBUILD_SAMPLES=OFF
      cmake --install .
    '';

    outputs = ["out"];

    meta = with lib; {
      homepage = "https://github.com/google/or-tools";
      license = licenses.asl20;
      description = ''
        Google's software suite for combinatorial optimization.
      '';
      mainProgram = "fzn-ortools";
      maintainers = with maintainers; [andersk];
      platforms = with platforms; linux ++ darwin;
    };
  })
