#!/usr/bin/env python3

import os
import shutil
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path


class CodeCoverageTest(unittest.TestCase):
    def test_static_bazel_uses_uncached_local_capture(self):
        result, archive_exists, commands = self._run("static-bazel")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(archive_exists)
        self.assertIn("bazelisk clean", commands)
        self.assertIn("cov-build --dir cov-int --bazel bazelisk build", commands)
        self.assertIn("--spawn_strategy=local", commands)
        self.assertIn("--remote_cache= --disk_cache=", commands)
        self.assertIn("--noremote_accept_cached", commands)
        self.assertIn("--noremote_upload_local_results", commands)
        self.assertIn("--//:platform=cli -- //:openroad", commands)
        self.assertNotIn("cmake ", commands)

    def test_static_keeps_the_cmake_capture(self):
        result, archive_exists, commands = self._run("static")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(archive_exists)
        self.assertIn("cmake -B build .", commands)
        self.assertIn("cmake --build build", commands)
        self.assertIn("cov-build --dir cov-int cmake --build build", commands)
        self.assertNotIn("bazelisk", commands)

    def test_static_fails_when_capture_percentage_is_missing(self):
        result, archive_exists, _ = self._run("static", build_log="no summary\n")

        self.assertEqual(result.returncode, 1)
        self.assertFalse(archive_exists)
        self.assertIn("Only got 0%", result.stdout)

    def test_static_fails_when_capture_percentage_is_low(self):
        result, archive_exists, _ = self._run(
            "static", build_log="Emitted 84 compilation units (84%)\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertFalse(archive_exists)
        self.assertIn("Only got 84%", result.stdout)

    def _run(self, mode, build_log="Emitted 100 compilation units (100%)\n"):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            etc = root / "repo" / "etc"
            fake_bin = root / "bin"
            etc.mkdir(parents=True)
            fake_bin.mkdir()

            script = etc / "CodeCoverage.sh"
            shutil.copy2(Path(__file__).with_name("CodeCoverage.sh"), script)

            command_log = root / "commands.log"
            self._write_executable(
                fake_bin / "bazelisk",
                '#!/bin/sh\nprintf "bazelisk %s\\n" "$*" >> "$COMMAND_LOG"\n',
            )
            self._write_executable(
                fake_bin / "cmake",
                '#!/bin/sh\nprintf "cmake %s\\n" "$*" >> "$COMMAND_LOG"\n',
            )
            self._write_executable(
                fake_bin / "cov-build",
                """#!/bin/sh
printf 'cov-build %s\n' "$*" >> "$COMMAND_LOG"
mkdir -p cov-int
cat "$BUILD_LOG_SOURCE" > cov-int/build-log.txt
""",
            )
            build_log_source = root / "build-log.txt"
            build_log_source.write_text(build_log)
            self._write_executable(
                fake_bin / "git",
                '#!/bin/sh\nprintf "0123456789abcdef\\n"\n',
            )

            env = os.environ.copy()
            env["COMMAND_LOG"] = str(command_log)
            env["BUILD_LOG_SOURCE"] = str(build_log_source)
            env["PATH"] = f"{fake_bin}:{env['PATH']}"
            env["SKIP_COVERITY_UPLOAD"] = "1"

            result = subprocess.run(
                [script, mode, "unused-test-token"],
                cwd=root / "repo",
                env=env,
                capture_output=True,
                text=True,
            )

            commands = command_log.read_text()
            archive = root / "repo" / "openroad.tgz"
            return result, archive.is_file(), commands

    @staticmethod
    def _write_executable(path, content):
        path.write_text(content)
        path.chmod(path.stat().st_mode | stat.S_IXUSR)


if __name__ == "__main__":
    unittest.main()
