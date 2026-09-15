#!/usr/bin/env python3

"""
Regression check for Bazel developer activities documentation.

Goal:
- Ensure the Bazel developer docs cover key day-to-day activities.
- Fail with clear messages when required topics are missing.
"""

import re
import sys
from typing import Dict, List
from pathlib import Path


def _norm(text: str) -> str:
    # Normalize for robust matching while preserving readability in errors.
    return re.sub(r"\s+", " ", text.lower())


def _contains_any(text: str, patterns: List[str]) -> bool:
    return any(re.search(p, text, flags=re.IGNORECASE) for p in patterns)


def main() -> int:
    base_dir = Path(__file__).resolve().parent.parent.parent  # docs/
    user_docs = base_dir / "user"

    required_files = [
        user_docs / "Bazel.md",
        user_docs / "Bazel-targets.md",
        user_docs / "Bazel-caching.md",
        user_docs / "Bazel-developer-activities.md",
    ]

    missing_files = [str(p) for p in required_files if not p.exists()]
    if missing_files:
        print("ERROR: Missing required Bazel documentation files:")
        for p in missing_files:
            print(f"  - {p}")
        return 1

    content_by_file: Dict[str, str] = {}
    for path in required_files:
        content_by_file[path.name] = _norm(path.read_text(encoding="utf-8"))

    checks = [
        (
            "bazel developer activities page",
            [r"bazel developer activities", r"\.bzl", r"starlark"],
            ["Bazel-developer-activities.md"],
        ),
        (
            "run local tests",
            [r"\bbazelisk\s+test\b", r"\bbazel\s+test\b"],
            ["Bazel.md"],
        ),
        (
            "run targeted test(s)",
            [r"\b//[^\s:]+:[^\s]+", r"\bsrc/[^\s]+/\.\.\."],
            ["Bazel.md"],
        ),
        (
            "build openroad binary",
            [r"\bbazelisk\s+build\s+:openroad\b", r"\bbazel\s+build\s+:openroad\b"],
            ["Bazel.md"],
        ),
        (
            "query/list tests",
            [r"\bbazelisk\s+query\b", r"\bbazel\s+query\b"],
            ["Bazel.md"],
        ),
        (
            "clean/rebuild guidance",
            [r"\bbazelisk\s+clean\b", r"\bbazel\s+clean\b"],
            ["Bazel.md"],
        ),
        (
            "host vs target configuration guidance",
            [r"\bcfg=exec\b", r"\bcfg=target\b", r"\bhost\b", r"\btarget\b"],
            ["Bazel-targets.md", "Bazel.md"],
        ),
        (
            "local cache guidance",
            [r"\bdisk[_ -]?cache\b", r"\blocal cache\b"],
            ["Bazel-caching.md"],
        ),
        (
            "remote cache guidance",
            [r"\bremote[_ -]?cache\b", r"https?://"],
            ["Bazel-caching.md"],
        ),
    ]

    failures = []
    for label, patterns, files in checks:
        found = False
        for fname in files:
            text = content_by_file.get(fname, "")
            if _contains_any(text, patterns):
                found = True
                break
        if not found:
            failures.append((label, files))

    if failures:
        print("ERROR: Bazel developer activities coverage check failed.")
        print("Missing coverage for:")
        for label, files in failures:
            print(f"  - {label} (expected in: {', '.join(files)})")
        return 1

    print("PASS: Bazel developer activities documentation coverage is adequate.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
