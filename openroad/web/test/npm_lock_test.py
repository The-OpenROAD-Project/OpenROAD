# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# rules_js installs what openroad/web/pnpm-lock.yaml says and never reads
# package.json, so a version bumped in package.json alone would go unnoticed.

import json
import unittest

import yaml

_FIX = "bazelisk run -- @pnpm//:pnpm --dir $PWD/openroad/web install --lockfile-only"


class LockMatchesManifest(unittest.TestCase):
    def setUp(self):
        with open("openroad/web/package.json", encoding="utf-8") as f:
            self.manifest = json.load(f)
        with open("openroad/web/pnpm-lock.yaml", encoding="utf-8") as f:
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

    def test_overrides(self):
        wanted = self.manifest.get("pnpm", {}).get("overrides", {})
        locked = self.lock.get("overrides") or {}
        self.assertEqual(locked, wanted, f"regenerate the lock: {_FIX}")


if __name__ == "__main__":
    unittest.main()
