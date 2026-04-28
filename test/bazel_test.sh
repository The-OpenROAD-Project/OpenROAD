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
	workspace_root="${TEST_SRCDIR}/${TEST_WORKSPACE:-_main}"
	python_paths="${workspace_root}"
	if [ -d "${workspace_root}/python" ]; then
		python_paths="${python_paths}:${workspace_root}/python"
	fi
	for src_dir in "${workspace_root}"/src/*; do
		if [ -d "${src_dir}" ]; then
			python_paths="${python_paths}:${src_dir}"
		fi
	done

	shim_dir="${TEST_TMPDIR}/python_shims"
	mkdir -p "${shim_dir}"

	write_openroad_shim() {
		cat >"${shim_dir}/$1.py" <<'EOF'
from openroadpy import *
EOF
	}

	if [ -f "${workspace_root}/src/utl/utl.py" ]; then
		cp "${workspace_root}/src/utl/utl.py" "${shim_dir}/utl_py.py"
		cat >"${shim_dir}/utl.py" <<'EOF'
from utl_py import *
EOF
	else
		cat >"${shim_dir}/utl.py" <<'EOF'
from src.utl.utl import *
EOF
	fi
	cat >"${shim_dir}/odb.py" <<'EOF'
from src.odb.odb import *
EOF
	cat >"${shim_dir}/gpl.py" <<'EOF'
from src.gpl.gpl import *
EOF
	cat >"${shim_dir}/rmp.py" <<'EOF'
try:
    from rmp_py import *
except ImportError:
    from openroadpy import *
EOF
	for module in ant cts drt grt rcx stt tap; do
		write_openroad_shim "${module}"
	done

	export PYTHONPATH="${shim_dir}:${python_paths}${PYTHONPATH:+:$PYTHONPATH}"
	if [ -n "${TEST_GOLDEN_FILE:-}" ]; then
		export TEST_GOLDEN_FILE="${workspace_root}/${TEST_GOLDEN_FILE}"
	fi
fi

cd $(dirname $TEST_FILE)
mkdir -p $RESULTS_DIR
$REGRESSION_TEST
