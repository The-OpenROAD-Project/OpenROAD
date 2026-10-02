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
        result, archive_exists, version, commands = self._run("static-bazel")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(archive_exists)
        self.assertEqual(version, "0123456789abcdef\n")
        self.assertIn("bazelisk clean", commands)
        self.assertIn("cov-build --dir cov-int --bazel bazelisk build", commands)
        self.assertIn("--spawn_strategy=local", commands)
        self.assertIn("--remote_cache= --disk_cache=", commands)
        self.assertIn("--noremote_accept_cached", commands)
        self.assertIn("--noremote_upload_local_results", commands)
        self.assertIn("--//:platform=cli -- //:openroad", commands)
        self.assertNotIn("cmake ", commands)

    def test_static_keeps_the_cmake_capture(self):
        result, archive_exists, version, commands = self._run("static")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(archive_exists)
        self.assertEqual(version, "0123456789abcdef\n")
        self.assertIn("cmake -B build .", commands)
        self.assertIn("cmake --build build", commands)
        self.assertIn("cov-build --dir cov-int cmake --build build", commands)
        self.assertNotIn("bazelisk", commands)

    def test_static_fails_when_capture_percentage_is_missing(self):
        result, archive_exists, version, _ = self._run(
            "static", build_log="no summary\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertFalse(archive_exists)
        self.assertIsNone(version)
        self.assertIn("Only got 0%", result.stdout)

    def test_static_fails_when_capture_percentage_is_low(self):
        result, archive_exists, version, _ = self._run(
            "static", build_log="Emitted 84 compilation units (84%)\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertFalse(archive_exists)
        self.assertIsNone(version)
        self.assertIn("Only got 84%", result.stdout)

    def test_upload_reuses_the_archive_without_running_a_capture(self):
        result, archive_exists, version, commands = self._run(
            "upload",
            artifact=True,
            version_file="fedcba9876543210\n",
            skip_upload=False,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(archive_exists)
        self.assertEqual(version, "fedcba9876543210\n")
        self.assertNotIn("cmake", commands)
        self.assertNotIn("cov-build", commands)
        self.assertIn("version=fedcba9876543210", commands)
        self.assertIn("--upload-file openroad.tgz", commands)
        self.assertIn("builds/825340/enqueue", commands)
        # curl 7.68 (Ubuntu 20.04) does not support --fail-with-body.
        self.assertNotIn("--fail-with-body", commands)

    def test_upload_accepts_a_version_for_a_legacy_archive(self):
        result, _, version, commands = self._run(
            "upload",
            artifact=True,
            version_arg="a432b134015160dd",
            skip_upload=False,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIsNone(version)
        self.assertIn("version=a432b134015160dd", commands)

    def test_upload_requires_an_archive(self):
        result, _, _, commands = self._run(
            "upload", version_file="fedcba9876543210\n", skip_upload=False
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("openroad.tgz does not exist", result.stderr)
        self.assertNotIn("curl", commands)

    def test_plain_text_initialization_error_is_reported(self):
        message = (
            "Your build is already in the queue for analysis. "
            "Please wait before uploading another build.\n"
        )
        result, archive_exists, version, commands = self._run(
            "static",
            skip_upload=False,
            init_response=message,
        )

        self.assertEqual(result.returncode, 1)
        self.assertTrue(archive_exists)
        self.assertEqual(version, "0123456789abcdef\n")
        self.assertIn(
            f"Coverity build initialization failed: {message.strip()}", result.stderr
        )
        self.assertNotIn("parse error", result.stderr)
        self.assertNotIn("--upload-file", commands)

    def _run(
        self,
        mode,
        build_log="Emitted 100 compilation units (100%)\n",
        *,
        artifact=False,
        version_file=None,
        version_arg=None,
        skip_upload=True,
        init_response='{"url":"https://upload.example/build","build_id":825340}\n',
    ):
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
            self._write_executable(
                fake_bin / "curl",
                """#!/bin/sh
printf 'curl %s\n' "$*" >> "$COMMAND_LOG"
case "$*" in
    *builds/init*) printf '%s' "$COVERITY_INIT_RESPONSE" ;;
esac
""",
            )
            repo = root / "repo"
            if artifact:
                (repo / "openroad.tgz").write_bytes(b"coverity archive")
            if version_file is not None:
                (repo / "openroad.version").write_text(version_file)

            env = os.environ.copy()
            env["COMMAND_LOG"] = str(command_log)
            env["BUILD_LOG_SOURCE"] = str(build_log_source)
            env["COVERITY_INIT_RESPONSE"] = init_response
            env["PATH"] = f"{fake_bin}:{env['PATH']}"
            if skip_upload:
                env["SKIP_COVERITY_UPLOAD"] = "1"
            else:
                env.pop("SKIP_COVERITY_UPLOAD", None)

            args = [script, mode, "unused-test-token"]
            if version_arg is not None:
                args.append(version_arg)

            result = subprocess.run(
                args,
                cwd=repo,
                env=env,
                capture_output=True,
                text=True,
            )

            commands = command_log.read_text() if command_log.exists() else ""
            archive = repo / "openroad.tgz"
            version_path = repo / "openroad.version"
            version = version_path.read_text() if version_path.exists() else None
            return result, archive.is_file(), version, commands

    @staticmethod
    def _write_executable(path, content):
        path.write_text(content)
        path.chmod(path.stat().st_mode | stat.S_IXUSR)


if __name__ == "__main__":
    unittest.main()
