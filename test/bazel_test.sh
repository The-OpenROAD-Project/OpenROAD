#!/usr/bin/env bash
set -ex
# TEST_NAME is used in cmake and bazel, but in bazel it is a label
# not a test name.
export TEST_EXT=${TEST_FILE##*.}
export TEST_NAME=$(basename "$TEST_FILE" .$TEST_EXT)
[ -n "$OPENROAD_EXE" ] && export OPENROAD_EXE=$(realpath $OPENROAD_EXE)
export RESULTS_DIR="${TEST_UNDECLARED_OUTPUTS_DIR}/results"
export REGRESSION_TEST=$(realpath $REGRESSION_TEST)
if [ -n "${TEST_SRCDIR:-}" ]; then
	export BAZEL_TEST=1
	# Tests cd into their source directory, so importing a shared helper
	# (helpers.py, ppl_aux.py, ...) makes CPython write __pycache__ .pyc
	# files back into the source tree mid-action. Bazel then sees a
	# concurrent modification and skips caching the result
	# (--guard_against_concurrent_changes). Suppress bytecode writing.
	export PYTHONDONTWRITEBYTECODE=1
	workspace_root="${TEST_SRCDIR}/${TEST_WORKSPACE:-_main}"
	# python/modules provides the per-tool modules (import odb, import ant,
	# ...) that the wheel installs at the top level.
	python_paths="${workspace_root}/python/modules:${workspace_root}"
	if [ -d "${workspace_root}/python" ]; then
		python_paths="${python_paths}:${workspace_root}/python"
	fi
	export PYTHONPATH="${python_paths}${PYTHONPATH:+:$PYTHONPATH}"
	if [ -n "${TEST_GOLDEN_FILE:-}" ]; then
		export TEST_GOLDEN_FILE="${workspace_root}/${TEST_GOLDEN_FILE}"
	fi
fi

cd $(dirname $TEST_FILE)
mkdir -p $RESULTS_DIR
$REGRESSION_TEST
