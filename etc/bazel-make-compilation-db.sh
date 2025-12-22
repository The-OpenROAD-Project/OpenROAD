#!/usr/bin/env bash

set -eu

# Which bazel and bant to use. If unset, defaults are used.
BAZEL=${BAZEL:-bazelisk}
if ! command -v "${BAZEL}">/dev/null ; then
  BAZEL=bazel
fi

BANT="$($(dirname "$0")/get-bant-path.sh)"

# Important to run with --remote_download_outputs=all to make sure generated
# files are actually visible locally in case a remote cache (that includes
# --disk_cache) is used ( https://github.com/hzeller/bazel-gen-file-issue )
BAZEL_OPTS="-c opt --remote_download_outputs=all"

# Tickle some build targets to fetch all dependencies and generate files,
# so that they can be seen by the users of the compilation db.
# Right now, comile everything (which should not be too taxing with the
# bazel cache), but it could be made more specific to only trigger specific
# targets that are sufficient to get and regenerate everything.
"${BAZEL}" build -k ${BAZEL_OPTS} src/...

# Create compilation DB. Command 'compilation-db' would create a huge *.json file,
# but compile_flags.txt is perfectly sufficient and easier for tools to use as
# these tools will require much less memory.

"${BANT}" compile-flags -o compile_flags.txt

# The QT headers are not properly picked up; add them manually.
for f in bazel-out/../../../external/qt-bazel*/qt_source/qtbase*/build/include/Q* ; do echo "-I$f" ; done >> compile_flags.txt

# Main.cc attemps to access this one.
echo '-DBUILD_TYPE="opt"' >> compile_flags.txt

# If there are two styles of comp-dbs, tools might have issues. Warn user.
if [ -r compile_commands.json ]; then
  printf "\n\033[1;31mSuggest to remove old compile_commands.json to not interfere with compile_flags.txt\033[0m\n\n"
fi
