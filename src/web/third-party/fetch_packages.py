#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Fetch the browser libraries the web viewer serves from the OpenROAD binary.
#
# The viewer must never load code from a CDN (issue #11065), and the libraries
# are not checked in, so the build fetches them: npm tarballs from the registry,
# every one verified against the sha256 in packages.json.  Bazel does this with
# its own downloader (//bazel:web_third_party.bzl); this script is the CMake
# side, and the shared esbuild invocation both use.
#
# Usage:
#   fetch_packages.py --dest DIR --emit-cmake FILE   # what CMake runs
#   fetch_packages.py --download-only --tarball-dir DIR
#   fetch_packages.py --bundle --esbuild BIN --tree DIR --entry FILE
#                     --entry-path FILE --output FILE   # the Bazel genrule
#   fetch_packages.py --print-sha256 leaflet@1.9.4   # when upgrading

import argparse
import hashlib
import io
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
MANIFEST_PATH = os.path.join(THIS_DIR, "packages.json")

# What uname calls this host -> what npm calls it.  The manifest owns which
# platforms are pinned; this only names the one we are running on.
ESBUILD_OS = {"Darwin": "darwin", "Linux": "linux"}
ESBUILD_ARCH = {"aarch64": "arm64", "arm64": "arm64", "x86_64": "x64"}


def read_manifest():
    with open(MANIFEST_PATH, encoding="utf-8") as f:
        return json.load(f)


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def tarball_url(registry, name, version):
    # Scoped names live at @scope/name/-/name-version.tgz.
    basename = name.split("/")[-1]
    return f"{registry}/{name}/-/{basename}-{version}.tgz"


def fetch(registry, name, version, expected, tarball_dir):
    """Return the package tarball, from tarball_dir if it is already there."""
    basename = f"{name.replace('/', '-').lstrip('-')}-{version}.tgz"
    cached = os.path.join(tarball_dir, basename) if tarball_dir else None

    if cached and os.path.exists(cached):
        with open(cached, "rb") as f:
            return verified(f.read(), name, version, expected)

    url = tarball_url(registry, name, version)
    print(f"  fetching {url}", file=sys.stderr)
    with urllib.request.urlopen(url, timeout=120) as response:
        data = verified(response.read(), name, version, expected)

    if cached:
        os.makedirs(tarball_dir, exist_ok=True)
        with open(cached, "wb") as f:
            f.write(data)
    return data


def verified(data, name, version, expected):
    digest = sha256_bytes(data)
    if digest != expected:
        raise SystemExit(
            f"{name}@{version} sha256 mismatch:\n"
            f"  expected {expected}\n  actual   {digest}\n"
            "The npm registry is immutable, so this is a wrong pin or a tampered "
            "download, never a stale one."
        )
    return data


def fetch_package(manifest, name, tarball_dir):
    spec = manifest["packages"][name]
    return fetch(
        manifest["registry"], name, spec["version"], spec["sha256"], tarball_dir
    )


def fetch_esbuild(manifest, tarball_dir):
    spec = manifest["esbuild"]
    platform_name = host_esbuild_platform()
    if platform_name not in spec["platforms"]:
        raise SystemExit(f"esbuild {spec['version']} for {platform_name} is not pinned")
    return fetch(
        manifest["registry"],
        f"@esbuild/{platform_name}",
        spec["version"],
        spec["platforms"][platform_name],
        tarball_dir,
    )


def extract(tar, member, root, rel):
    """Extract one member to root/rel, and return where it landed.

    A bundled tree takes rel from the tarball, so it has to be shown to stay
    under root.
    """
    if not member.isfile():
        raise SystemExit(f"{member.name} is not a regular file")

    root = os.path.realpath(root)
    dest = os.path.realpath(os.path.join(root, rel))
    if not dest.startswith(root + os.sep):
        raise SystemExit(f"{member.name} would write outside {root}")

    os.makedirs(os.path.dirname(dest), exist_ok=True)
    with tar.extractfile(member) as src, open(dest, "wb") as out:
        shutil.copyfileobj(src, out)
    return dest


def open_tarball(data):
    return tarfile.open(fileobj=io.BytesIO(data))


def host_esbuild_platform():
    system = ESBUILD_OS.get(platform.system())
    arch = ESBUILD_ARCH.get(platform.machine())
    if not system or not arch:
        raise SystemExit(
            f"no esbuild binary is pinned for {platform.system()}/"
            f"{platform.machine()}.  Add the platform to esbuild.platforms in "
            "packages.json and to the maps at the top of this script."
        )
    return f"{system}-{arch}"


def install_esbuild(manifest, dest, tarball_dir):
    """Unpack the pinned esbuild for this host and return its path."""
    binary = os.path.join(dest, ".esbuild", "esbuild")
    with open_tarball(fetch_esbuild(manifest, tarball_dir)) as tar:
        extract(tar, tar.getmember("package/bin/esbuild"), dest, ".esbuild/esbuild")
    os.chmod(binary, os.stat(binary).st_mode | stat.S_IXUSR)
    return binary


def run_esbuild(esbuild, tree_root, entry, output):
    """Bundle an ES module tree, from a directory laid out as the package ships.

    esbuild names each module in a comment, by the path it reached it through,
    so tree_root has to be a real directory holding the tree at its published
    relative path -- not a symlink farm, whose name would land in the output.
    """
    output = os.path.abspath(output)
    os.makedirs(os.path.dirname(output), exist_ok=True)
    subprocess.run(
        [
            os.path.abspath(esbuild),
            entry,
            "--bundle",
            "--format=esm",
            "--legal-comments=inline",
            f"--outfile={output}",
        ],
        cwd=tree_root,
        check=True,
    )


def bundle_staged_tree(esbuild, tree, entry, entry_path, output):
    """Bundle a tree the build system staged, wherever it put it.

    Bazel hands the tree over as symlinks into its repository cache, so it is
    copied to a scratch directory first; the entry's path is what says where
    the staged tree starts.
    """
    entry_path = os.path.abspath(entry_path)
    suffix = os.sep + entry.replace("/", os.sep)
    if not entry_path.endswith(suffix):
        raise SystemExit(
            f"{entry_path} does not end in {entry}, so the tree root cannot be "
            "derived from it"
        )
    staged = os.path.join(entry_path[: -len(suffix)], tree)
    with tempfile.TemporaryDirectory() as work_dir:
        shutil.copytree(staged, os.path.join(work_dir, tree), symlinks=False)
        run_esbuild(esbuild, work_dir, entry, output)


def asset_paths(manifest, dest):
    """Served path -> file path, without fetching anything.

    A pure function of the manifest, so the emitted CMake can be written on the
    reuse path too rather than being trusted from a previous run.
    """
    assets = {}
    for spec in manifest["packages"].values():
        served = list(spec["files"].values())
        if "bundle" in spec:
            served.append(spec["bundle"]["output"])
        for path in served:
            assets["/third-party/" + path] = os.path.join(dest, path)
    return assets


def populate(manifest, dest, tarball_dir):
    """Extract every package into dest, and bundle the ones that need it."""
    needs_esbuild = any("bundle" in s for s in manifest["packages"].values())
    esbuild = install_esbuild(manifest, dest, tarball_dir) if needs_esbuild else None

    for name, spec in manifest["packages"].items():
        print(f"{name}@{spec['version']}", file=sys.stderr)
        with open_tarball(fetch_package(manifest, name, tarball_dir)) as tar:
            for source in spec["files"]:
                extract(
                    tar,
                    tar.getmember("package/" + source),
                    dest,
                    spec["files"][source],
                )
            if "bundle" in spec:
                bundle_from_tarball(esbuild, tar, spec["bundle"], dest)


def bundle_from_tarball(esbuild, tar, spec, dest):
    """Extract a module tree straight into the directory esbuild runs in."""
    prefix = "package/" + spec["tree"] + "/"
    with tempfile.TemporaryDirectory() as work_dir:
        for member in tar.getmembers():
            # esbuild is not asked for a source map, so the .map files that make
            # up a third of the tree would only be extracted for nothing.
            if member.isfile() and member.name.startswith(prefix):
                if not member.name.endswith(".map"):
                    extract(tar, member, work_dir, member.name[len("package/") :])
        run_esbuild(
            esbuild, work_dir, spec["entry"], os.path.join(dest, spec["output"])
        )


def stamp_value():
    """What has to be equal for an existing dest to be reusable.

    The script is in there with the manifest: it decides what the fetch
    produces, so changing how a package is extracted or bundled has to
    invalidate a tree just as changing which package does.
    """
    parts = []
    for path in (MANIFEST_PATH, os.path.abspath(__file__)):
        with open(path, "rb") as f:
            parts.append(f.read())
    parts.append(host_esbuild_platform().encode())
    return sha256_bytes(b"".join(parts))


def emit_cmake(path, assets):
    items = sorted(assets.items())
    lines = ["# Generated by fetch_packages.py -- do not edit."]
    lines.append("set(WEB_THIRD_PARTY_ASSET_ARGS")
    lines += [f'  "{served}={file}"' for served, file in items]
    lines.append(")")
    lines.append("set(WEB_THIRD_PARTY_FILES")
    lines += [f'  "{file}"' for _, file in items]
    lines += [")", ""]
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dest", help="directory to extract the packages into")
    parser.add_argument("--emit-cmake", help="write the asset lists as CMake code")
    parser.add_argument(
        "--tarball-dir",
        help="look for the tarballs here before downloading, and cache them here; "
        "point at a primed directory to build with no network",
    )
    parser.add_argument(
        "--download-only",
        action="store_true",
        help="fill --tarball-dir and exit, so a later build needs no network",
    )
    parser.add_argument(
        "--bundle", action="store_true", help="bundle a staged module tree and exit"
    )
    parser.add_argument("--esbuild", help="esbuild binary, with --bundle")
    parser.add_argument("--tree", help="the tree's path in the package, with --bundle")
    parser.add_argument("--entry", help="the entry's path in the package")
    parser.add_argument("--entry-path", help="where the build staged that entry")
    parser.add_argument("--output", help="the bundle to write, with --bundle")
    parser.add_argument(
        "--print-sha256", help="print the digest of <package>@<version>"
    )
    args = parser.parse_args()

    if args.print_sha256:
        manifest = read_manifest()
        name, _, version = args.print_sha256.rpartition("@")
        url = tarball_url(manifest["registry"], name, version)
        with urllib.request.urlopen(url, timeout=120) as response:
            print(sha256_bytes(response.read()))
        return 0

    if args.bundle:
        missing = [
            flag
            for flag, value in (
                ("--esbuild", args.esbuild),
                ("--tree", args.tree),
                ("--entry", args.entry),
                ("--entry-path", args.entry_path),
                ("--output", args.output),
            )
            if not value
        ]
        if missing:
            raise SystemExit("--bundle needs " + ", ".join(missing))
        bundle_staged_tree(
            args.esbuild, args.tree, args.entry, args.entry_path, args.output
        )
        return 0

    manifest = read_manifest()

    if args.download_only:
        if not args.tarball_dir:
            raise SystemExit("--download-only needs --tarball-dir")
        for name in manifest["packages"]:
            fetch_package(manifest, name, args.tarball_dir)
        fetch_esbuild(manifest, args.tarball_dir)
        return 0

    if not args.dest:
        raise SystemExit("--dest is required")

    # The emitted paths are read from another directory, so they have to be
    # absolute however this was invoked.
    dest = os.path.abspath(args.dest)
    stamp = stamp_value()
    stamp_path = os.path.join(dest, ".stamp")
    current = os.path.exists(stamp_path)
    if current:
        with open(stamp_path, encoding="utf-8") as f:
            current = f.read().strip() == stamp

    if not current:
        if os.path.exists(dest):
            # The refresh below wipes dest, so only a directory a previous run
            # left its stamp in may be given: an in-source build, or
            # WEB_THIRD_PARTY pointed here, would otherwise take this script and
            # the manifest with it.
            if not os.path.exists(stamp_path):
                raise SystemExit(
                    f"{dest} already exists and was not written by this script "
                    "(no .stamp), and --dest is emptied before it is filled. "
                    "Point it at a build directory."
                )
            shutil.rmtree(dest)
        os.makedirs(dest)
        populate(manifest, dest, args.tarball_dir)
        with open(stamp_path, "w", encoding="utf-8") as f:
            f.write(stamp + "\n")

    if args.emit_cmake:
        emit_cmake(os.path.abspath(args.emit_cmake), asset_paths(manifest, dest))
    return 0


if __name__ == "__main__":
    sys.exit(main())
