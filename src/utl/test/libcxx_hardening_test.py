# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import signal
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    result = subprocess.run([sys.argv[1]], capture_output=True, text=True)
    assert result.returncode == -signal.SIGABRT, result
    assert "Stack trace:" in result.stderr, result.stderr
    assert "libc++ check failed:" in result.stderr, result.stderr

    with tempfile.TemporaryDirectory() as directory:
        log_file = Path(directory) / "hardening.log"
        logged = subprocess.run(
            [sys.argv[1], str(log_file)], capture_output=True, text=True
        )
        assert logged.returncode == 1, logged
        contents = log_file.read_text()
        assert "Stack trace:\ntest frame" in contents, contents
        assert (
            "[CRITICAL ORD-2057] libc++ check failed: test condition" in contents
        ), contents


if __name__ == "__main__":
    main()
