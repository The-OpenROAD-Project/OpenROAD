###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2021, The Regents of the University of California
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


if (NOT USE_SYSTEM_ABC)

  set(ABC_USE_NAMESPACE "abc")

  # abc does not have "platform" support for macos so force it to use stdint.h
  if (CMAKE_OSX_SYSROOT)
     set(ABC_USE_STDINT_H 1)
  endif()

  # make sure not to include readline headers to avoid extra dependencies
  # readline is not needed since we call abc from c++
  set(READLINE_FOUND FALSE)

# apple clang 14.0.0 seg faults on abc without -g
add_compile_options(
  $<$<CXX_COMPILER_ID:AppleClang>:-g>
)

  set(ABC_SKIP_TESTS TRUE CACHE BOOL "Disable tests in the ABC submodule")
  add_subdirectory(abc)

  # kissat (under abc/src/sat/kissat) casts STACK(T)* to chars* when calling
  # kissat_stack_enlarge — see third-party/abc/src/sat/kissat/stack.h:41. GCC's
  # type-based alias analysis treats the call as unable to modify the original
  # stack, caches the stale end pointer across the realloc inside enlarge, and
  # dereferences NULL on the first push.
  target_compile_options(libabc PRIVATE -fno-strict-aliasing)
  target_compile_options(libabc-pic PRIVATE -fno-strict-aliasing)

endif()

add_subdirectory(lodepng)
add_subdirectory(linenoise)
add_subdirectory(stb_truetype)

# --- slang frontend (for src/syn elaboration) ---

# Disable slang extras we don't need; reuse slang-elab's bundled fmt.
set(SLANG_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_TOOLS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_INSTALL OFF CACHE BOOL "" FORCE)
set(SLANG_USE_MIMALLOC OFF CACHE BOOL "" FORCE)

add_subdirectory(slang-elab/third_party/slang EXCLUDE_FROM_ALL)

# slang's gitversion.cmake defines a colliding get_git_head_revision() function
# that later breaks STA's call (_gitversionmoddir is unset in STA's dir scope).
# Reload OpenROAD's GetGitRevisionDescription to restore the correct version.
unset(__get_git_revision_description)
include(GetGitRevisionDescription)

# Build slang-elab's frontend-only library (slang_frontend); skip the Yosys
# plugin, which we don't build in the OpenROAD tree.
set(SLANG_ELAB_BUILD_YOSYS_PLUGIN OFF CACHE BOOL "" FORCE)

find_package(fmt CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/slang-elab/cmake")
add_subdirectory(slang-elab/src EXCLUDE_FROM_ALL)
