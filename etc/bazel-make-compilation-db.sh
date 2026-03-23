#!/usr/bin/env bash
# Environment variables
#   BAZEL      : the bazel binary to invoke (default: baselisk)
#   BAZEL_OPTS : the preferred bazel options used
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
BAZEL_REMOTE_MATERIALIZE=--remote_download_outputs=all

# Bazel options
BAZEL_OPTS="${BAZEL_OPTS:--c opt ${BAZEL_REMOTE_MATERIALIZE}}"

# Tickle some build targets to fetch all dependencies and generate files,
# so that they can be seen by the users of the compilation db.
# (Note, we need to limit targets to everything below //src, otherwise
# it requires docker)
"${BAZEL}" fetch "${BAZEL_REMOTE_MATERIALIZE}" //src/...
"${BAZEL}" build -k ${BAZEL_OPTS} \
  @openmp//:omp_header \
  $("${BANT}" list-targets -g "genrule|tcl_encode|tcl_wrap_cc" -g "//src" | awk '{print $3}')

# Create compilation DB. Command 'compilation-db' would create a huge *.json file,
# but compile_flags.txt is perfectly sufficient and easier for tools to use as
# these tools will require much less memory.

"${BANT}" compile-flags -o compile_flags.txt

# The QT headers are not properly picked up; add them manually.
for f in bazel-out/../../../external/qt-bazel*/qt_source/qtbase*/build/include \
  bazel-out/../../../external/qt-bazel*/qt_source/qtbase*/build/include/Q* \
  bazel-out/../../../external/qt-bazel*/qt_source/qtcharts*/include/QtCharts
do
  echo "-I$f"
  # There is a bug in clangd that does not properly follow symbolic links.
  # So expand them here as well for interactive use.
  echo "-I$(realpath $f)"
done >> compile_flags.txt

# Qt include files check for this
echo '-fPIC' >> compile_flags.txt

# Python include bindings.
for f in bazel-out/../../../external/*/include/python3.*/Python.h; do
  if [ -f "${f}" ]; then
    PY_INC="$(dirname "${f}")"
    echo "-I${PY_INC}"
    echo "-I$(realpath "${PY_INC}")"  # work around clangd bug
    break
  fi
done >> compile_flags.txt

# Since we don't do per-file define extraction in compile_flag.txt,
# add them here globally
cat >> compile_flags.txt <<EOF
-DABC_USE_STDINT_H=1
-DABC_NAMESPACE=abc
-DGPU=false
-DBUILD_TYPE="opt"
-DBUILD_PYTHON=false
-DBUILD_GUI=true
EOF

# If there are two styles of comp-dbs, tools might have issues. Warn user.
if [ -r compile_commands.json ]; then
  printf "\n\033[1;31mSuggest to remove old compile_commands.json to not interfere with compile_flags.txt\033[0m\n\n"
fi
