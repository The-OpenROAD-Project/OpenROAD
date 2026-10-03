# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# rules_js installs what src/web/pnpm-lock.yaml says and never reads
# package.json, so a version bumped in package.json alone would go unnoticed.

import json
import unittest

import yaml

_FIX = "bazel run -- @pnpm//:pnpm --dir $PWD/src/web install --lockfile-only"


class LockMatchesManifest(unittest.TestCase):
    def setUp(self):
        with open("src/web/package.json", encoding="utf-8") as f:
            self.manifest = json.load(f)
        with open("src/web/pnpm-lock.yaml", encoding="utf-8") as f:
            self.lock = yaml.safe_load(f)

    def test_specifiers(self):
        wanted = {
            **self.manifest.get("dependencies", {}),
            **self.manifest.get("devDependencies", {}),
        }
        importer = self.lock["importers"]["."]
        locked = {
            name: entry["specifier"]
            for section in ("dependencies", "devDependencies")
            for name, entry in (importer.get(section) or {}).items()
        }
        self.assertEqual(locked, wanted, f"regenerate the lock: {_FIX}")

    def test_patches(self):
        wanted = self.manifest.get("pnpm", {}).get("patchedDependencies", {})
        locked = self.lock.get("patchedDependencies") or {}
        self.assertEqual(sorted(locked), sorted(wanted), f"regenerate the lock: {_FIX}")
        for name, entry in locked.items():
            # pnpm 10 records the path next to the hash; later versions do not.
            if isinstance(entry, dict):
                self.assertEqual(entry["path"], wanted[name])


if __name__ == "__main__":
    unittest.main()
