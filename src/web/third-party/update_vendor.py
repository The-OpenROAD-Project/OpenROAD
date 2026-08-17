#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Refresh the vendored browser assets under src/web/third-party.
#
# The viewer serves them from the OpenROAD binary and must never fetch code from
# a CDN (issue #11065), so they are checked in and this script is what produces
# them: run by hand on an upgrade, never by the build.  Everything comes from the
# npm registry and every byte is verified against vendor.lock.json.  golden-layout
# is the one package with no browser bundle, so esbuild -- itself downloaded and
# pinned -- makes one.
#
# Usage:
#   ./update_vendor.py                # re-download and rewrite the tree
#   ./update_vendor.py --check        # verify the tree against the lock
#                                     # (offline; run by CI and code review)
#   ./update_vendor.py --rewrite-lock # after bumping a version below

import argparse
import hashlib
import json
import os
import platform
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
LOCK_PATH = os.path.join(THIS_DIR, "vendor.lock.json")
REGISTRY = "https://registry.npmjs.org"

# Files this directory owns that no package provides.
NON_VENDORED = frozenset(["update_vendor.py", "vendor.lock.json", "README.md"])

_GOLDEN_LAYOUT_IMAGES = [
    "lm_close_black.png",
    "lm_close_tab_white.png",
    "lm_close_white.png",
    "lm_maximise_black.png",
    "lm_maximise_white.png",
    "lm_minimize_black.png",
    "lm_minimize_white.png",
    "lm_popin_black.png",
    "lm_popin_white.png",
    "lm_popout_black.png",
    "lm_popout_white.png",
]

# name -> version, plus the members to copy out of the tarball as
# <path in tarball, without "package/"> -> <path under this dir>.  Each package's
# layout is preserved: the stylesheets reach their icons through relative urls.
PACKAGES = {
    "leaflet": {
        "version": "1.9.4",
        "files": {
            "LICENSE": "leaflet/LICENSE",
            "dist/leaflet.js": "leaflet/leaflet.js",
            "dist/leaflet.css": "leaflet/leaflet.css",
            **{
                f"dist/images/{name}": f"leaflet/images/{name}"
                for name in [
                    "layers.png",
                    "layers-2x.png",
                    "marker-icon.png",
                    "marker-icon-2x.png",
                    "marker-shadow.png",
                ]
            },
        },
    },
    "golden-layout": {
        "version": "2.6.0",
        "files": {
            "LICENSE": "golden-layout/LICENSE",
            "dist/css/goldenlayout-base.css": "golden-layout/css/goldenlayout-base.css",
            "dist/css/themes/goldenlayout-dark-theme.css": (
                "golden-layout/css/themes/goldenlayout-dark-theme.css"
            ),
            "dist/css/themes/goldenlayout-light-theme.css": (
                "golden-layout/css/themes/goldenlayout-light-theme.css"
            ),
            **{
                f"dist/img/{name}": f"golden-layout/img/{name}"
                for name in _GOLDEN_LAYOUT_IMAGES
            },
        },
        # No browser build is published, and the saved report needs one file: a
        # relative import does not resolve from a data: URL.
        "bundle": {
            "tree": "dist/esm/",
            "entry": "dist/esm/index.js",
            "output": "golden-layout/golden-layout.esm.js",
        },
    },
    "three": {
        "version": "0.160.0",
        "files": {
            "LICENSE": "three/LICENSE",
            "build/three.module.min.js": "three/three.module.min.js",
        },
    },
    # netlistsvg picks ELK up from the global scope rather than bundling it, so
    # elk.bundled.js has to be vendored too and loaded first.
    "elkjs": {
        "version": "0.9.3",
        "files": {
            "LICENSE.md": "elkjs/LICENSE.md",
            "lib/elk.bundled.js": "elkjs/elk.bundled.js",
        },
    },
    "netlistsvg": {
        "version": "1.0.2",
        "files": {
            "LICENSE": "netlistsvg/LICENSE",
            "built/netlistsvg.bundle.js": "netlistsvg/netlistsvg.bundle.js",
        },
    },
}

ESBUILD_VERSION = "0.28.2"

# uname -> the @esbuild package that carries the matching binary.
ESBUILD_PLATFORMS = {
    ("Linux", "x86_64"): "linux-x64",
    ("Linux", "aarch64"): "linux-arm64",
    ("Linux", "arm64"): "linux-arm64",
    ("Darwin", "x86_64"): "darwin-x64",
    ("Darwin", "arm64"): "darwin-arm64",
}


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256_file(path):
    with open(path, "rb") as f:
        return sha256_bytes(f.read())


def read_lock():
    if not os.path.exists(LOCK_PATH):
        return {"packages": {}, "tools": {}, "files": {}}
    with open(LOCK_PATH, encoding="utf-8") as f:
        return json.load(f)


def write_lock(lock):
    with open(LOCK_PATH, "w", encoding="utf-8") as f:
        json.dump(lock, f, indent=2, sort_keys=True)
        f.write("\n")


def tarball_url(name, version):
    # Scoped names live at @scope/name/-/name-version.tgz.
    basename = name.split("/")[-1]
    return f"{REGISTRY}/{name}/-/{basename}-{version}.tgz"


def download(name, version):
    url = tarball_url(name, version)
    print(f"  fetching {url}")
    with urllib.request.urlopen(url, timeout=120) as response:
        return response.read()


def vendored_files():
    """Every file under this directory that a package is expected to provide."""
    found = []
    for dirpath, dirnames, filenames in os.walk(THIS_DIR):
        dirnames[:] = [name for name in dirnames if name != "__pycache__"]
        for filename in filenames:
            rel = os.path.relpath(os.path.join(dirpath, filename), THIS_DIR)
            if rel not in NON_VENDORED:
                found.append(rel)
    return sorted(found)


def pinned_versions(lock):
    """What the lock says is pinned, against what the script asks for."""
    wanted = {name: spec["version"] for name, spec in PACKAGES.items()}
    wanted["esbuild"] = ESBUILD_VERSION
    locked = {
        name: lock.get("packages", {}).get(name, {}).get("version") for name in PACKAGES
    }
    locked["esbuild"] = lock.get("tools", {}).get("esbuild", {}).get("version")
    return [
        f"{name} is pinned at {locked[name]} but the script asks for {version}; "
        "re-run with --rewrite-lock"
        for name, version in sorted(wanted.items())
        if locked[name] != version
    ]


def check(lock):
    """Verify the checked-in tree byte for byte against the lock."""
    expected = lock.get("files", {})
    if not expected:
        print("vendor.lock.json lists no files", file=sys.stderr)
        return 1

    # The digests say the tree matches the lock; this says the lock matches what
    # the script would fetch today.
    problems = pinned_versions(lock)

    for rel, digest in sorted(expected.items()):
        path = os.path.join(THIS_DIR, rel)
        if not os.path.exists(path):
            problems.append(f"missing: {rel}")
            continue
        actual = sha256_file(path)
        if actual != digest:
            problems.append(
                f"modified: {rel}\n  expected {digest}\n  actual   {actual}"
            )

    for rel in vendored_files():
        if rel not in expected:
            problems.append(f"not in lock: {rel}")

    if problems:
        print("\n".join(problems), file=sys.stderr)
        print(
            "\nThe vendored files are generated: edit update_vendor.py and re-run it "
            "rather than patching them by hand.",
            file=sys.stderr,
        )
        return 1

    print(f"{len(expected)} vendored files match vendor.lock.json")
    return 0


def extract_member(tar, name, root, rel):
    """Extract one file of the tarball to root/rel, and return where it landed.

    A bundled tree takes rel from the tarball, so it has to be shown to stay
    under root -- on a --rewrite-lock run no digest would have caught it.
    """
    member = tar.getmember(name)
    if not member.isfile():
        raise RuntimeError(f"{name} is not a regular file")

    root = os.path.realpath(root)
    dest = os.path.realpath(os.path.join(root, rel))
    if not dest.startswith(root + os.sep):
        raise RuntimeError(f"{name} would write outside {root}")

    os.makedirs(os.path.dirname(dest), exist_ok=True)
    with tar.extractfile(member) as src, open(dest, "wb") as out:
        shutil.copyfileobj(src, out)
    return dest


def esbuild_binary(lock, work_dir, rewrite_lock):
    """Download the pinned esbuild for this host and return its path."""
    key = (platform.system(), platform.machine())
    if key not in ESBUILD_PLATFORMS:
        raise RuntimeError(
            f"no esbuild mapping for {key[0]}/{key[1]}; add it to ESBUILD_PLATFORMS "
            "and re-run with --rewrite-lock"
        )
    plat = ESBUILD_PLATFORMS[key]

    tools = lock.setdefault("tools", {}).setdefault("esbuild", {})
    pinned = (
        tools.get("platforms", {}) if tools.get("version") == ESBUILD_VERSION else {}
    )
    downloaded = {}
    if rewrite_lock:
        # Every platform, not only this host's: otherwise the first run
        # elsewhere has nothing to verify the binary against.
        for name in sorted(set(ESBUILD_PLATFORMS.values())):
            downloaded[name] = download(f"@esbuild/{name}", ESBUILD_VERSION)
        pinned = {name: sha256_bytes(data) for name, data in downloaded.items()}
        tools["version"] = ESBUILD_VERSION
        tools["platforms"] = pinned
    if plat not in pinned:
        raise RuntimeError(
            f"esbuild {ESBUILD_VERSION} for {plat} is not pinned in vendor.lock.json; "
            "re-run with --rewrite-lock to record it"
        )

    data = downloaded.get(plat) or download(f"@esbuild/{plat}", ESBUILD_VERSION)
    digest = sha256_bytes(data)
    if digest != pinned[plat]:
        raise RuntimeError(
            f"esbuild {plat} sha256 mismatch: expected {pinned[plat]}, got {digest}"
        )

    archive = os.path.join(work_dir, "esbuild.tgz")
    with open(archive, "wb") as f:
        f.write(data)
    with tarfile.open(archive) as tar:
        binary = extract_member(tar, "package/bin/esbuild", work_dir, "esbuild")
    os.chmod(binary, os.stat(binary).st_mode | stat.S_IXUSR)
    return binary


def bundle(spec, tar, lock, work_dir, rewrite_lock):
    """Bundle a package's ES module tree into one file with esbuild."""
    tree_dir = os.path.join(work_dir, "tree")
    prefix = "package/" + spec["tree"]
    for member in tar.getmembers():
        if member.isfile() and member.name.startswith(prefix):
            rel = member.name[len("package/") :]
            extract_member(tar, member.name, tree_dir, rel)

    if not os.path.exists(os.path.join(tree_dir, spec["entry"])):
        raise RuntimeError(f"bundle entry point not found: {spec['entry']}")

    output = os.path.join(THIS_DIR, spec["output"])
    os.makedirs(os.path.dirname(output), exist_ok=True)
    # Relative entry point, run from the tree: esbuild writes module paths into
    # comments, so an absolute one would differ from run to run.
    subprocess.run(
        [
            esbuild_binary(lock, work_dir, rewrite_lock),
            spec["entry"],
            "--bundle",
            "--format=esm",
            "--legal-comments=inline",
            f"--outfile={output}",
        ],
        cwd=tree_dir,
        check=True,
    )
    return spec["output"]


def update(lock, rewrite_lock):
    # Decided before the first byte moves, so a bump without --rewrite-lock does
    # not leave the tree half-rewritten.
    if not rewrite_lock:
        stale = pinned_versions(lock)
        if stale:
            print("\n".join(stale), file=sys.stderr)
            return 1

    written = []
    for name, spec in PACKAGES.items():
        version = spec["version"]
        print(f"{name}@{version}")
        data = download(name, version)

        digest = sha256_bytes(data)
        entry = lock.setdefault("packages", {}).setdefault(name, {})
        pinned = entry.get("sha256") if entry.get("version") == version else None
        if pinned != digest and not rewrite_lock:
            print(
                f"  tarball sha256 mismatch for {name}@{version}:\n"
                f"    expected {pinned}\n    actual   {digest}\n"
                "  The registry is immutable, so this means the lock is stale or "
                "the download was tampered with.  Re-run with --rewrite-lock only "
                "if you just changed the version.",
                file=sys.stderr,
            )
            return 1
        entry["version"] = version
        entry["sha256"] = digest

        with tempfile.TemporaryDirectory() as work_dir:
            archive = os.path.join(work_dir, "package.tgz")
            with open(archive, "wb") as f:
                f.write(data)
            with tarfile.open(archive) as tar:
                for source, dest in spec["files"].items():
                    extract_member(tar, "package/" + source, THIS_DIR, dest)
                    written.append(dest)
                if "bundle" in spec:
                    written.append(
                        bundle(spec["bundle"], tar, lock, work_dir, rewrite_lock)
                    )

    stale = set(vendored_files()) - set(written)
    for rel in sorted(stale):
        print(f"  removing stale file: {rel}")
        os.remove(os.path.join(THIS_DIR, rel))

    lock["files"] = {rel: sha256_file(os.path.join(THIS_DIR, rel)) for rel in written}
    write_lock(lock)
    print(f"wrote {len(written)} files and vendor.lock.json")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify the checked-in tree against vendor.lock.json and exit",
    )
    parser.add_argument(
        "--rewrite-lock",
        action="store_true",
        help="accept new tarball digests (use after bumping a version)",
    )
    args = parser.parse_args()

    lock = read_lock()
    if args.check:
        return check(lock)
    return update(lock, args.rewrite_lock)


if __name__ == "__main__":
    sys.exit(main())
