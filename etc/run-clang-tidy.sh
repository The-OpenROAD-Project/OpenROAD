#!/usr/bin/env bash

# Invocation without parameters simply uses the .clang-tidy config to run on
# all *.{cc,h} files. Additional parameters passed to this script are passed
# to clang-tidy as-is, e.g. if you only want to run a particular check and
# the auto-fix
#   run-clang-tidy.sh --checks="-*,modernize-loop-convert" --fix

set -eu

BAZEL="${BAZEL:-bazelisk}"
if ! command -v "${BAZEL}">/dev/null ; then
  BAZEL=bazel
fi

# Use either CLANG_TIDY provided by the user as environment variable or use
# our own from the toolchain we configured in the MODULE.bazel
export CLANG_TIDY="${CLANG_TIDY:-$("${BAZEL}" run -c opt --run_under='echo' @llvm_toolchain//:clang-tidy 2>/dev/null)}"

# The user should keep the compilation DB fresh, but refresh here
# if substantial things changed or it is not there in the first place.
if [ MODULE.bazel -nt compile_flags.txt -o BUILD.bazel -nt compile_flags.txt ] ; then
   "$(dirname "$0")/bazel-make-compilation-db.sh"
fi

# We don't want to accidentally load a compile_commands.json which
# is slow and might come from the other build system.
# Still allow user to override
export COMPILE_JSON="${COMPILE_JSON:-compile_flags.txt}"

"$(dirname "$0")/run-clang-tidy-cached.cc" "$@"
