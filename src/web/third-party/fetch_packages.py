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
#   fetch_packages.py --bundle golden-layout --esbuild BIN --entry FILE
#                     --output FILE                  # what the Bazel genrule runs
#   fetch_packages.py --print-sha256 leaflet@1.9.4   # when upgrading

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
MANIFEST_PATH = os.path.join(THIS_DIR, "packages.json")

# uname -> the @esbuild package that carries the matching binary.
ESBUILD_PLATFORMS = {
    ("Linux", "x86_64"): "linux-x64",
    ("Linux", "aarch64"): "linux-arm64",
    ("Linux", "arm64"): "linux-arm64",
    ("Darwin", "x86_64"): "darwin-x64",
    ("Darwin", "arm64"): "darwin-arm64",
}


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
            data = f.read()
    else:
        url = tarball_url(registry, name, version)
        print(f"  fetching {url}", file=sys.stderr)
        with urllib.request.urlopen(url, timeout=120) as response:
            data = response.read()

    digest = sha256_bytes(data)
    if digest != expected:
        raise SystemExit(
            f"{name}@{version} sha256 mismatch:\n"
            f"  expected {expected}\n  actual   {digest}\n"
            "The npm registry is immutable, so this is a wrong pin or a tampered "
            "download, never a stale one."
        )

    if cached and not os.path.exists(cached):
        os.makedirs(tarball_dir, exist_ok=True)
        with open(cached, "wb") as f:
            f.write(data)
    return data


def extract_member(tar, name, root, rel):
    """Extract one file of the tarball to root/rel, and return where it landed.

    A bundled tree takes rel from the tarball, so it has to be shown to stay
    under root.
    """
    member = tar.getmember(name)
    if not member.isfile():
        raise SystemExit(f"{name} is not a regular file")

    root = os.path.realpath(root)
    dest = os.path.realpath(os.path.join(root, rel))
    if not dest.startswith(root + os.sep):
        raise SystemExit(f"{name} would write outside {root}")

    os.makedirs(os.path.dirname(dest), exist_ok=True)
    with tar.extractfile(member) as src, open(dest, "wb") as out:
        shutil.copyfileobj(src, out)
    return dest


def host_esbuild_platform():
    key = (platform.system(), platform.machine())
    if key not in ESBUILD_PLATFORMS:
        raise SystemExit(
            f"no esbuild binary is pinned for {key[0]}/{key[1]}.  Add the platform "
            "to ESBUILD_PLATFORMS here and to esbuild.platforms in packages.json."
        )
    return ESBUILD_PLATFORMS[key]


def install_esbuild(manifest, dest, tarball_dir):
    """Unpack the pinned esbuild for this host and return its path."""
    spec = manifest["esbuild"]
    plat = host_esbuild_platform()
    if plat not in spec["platforms"]:
        raise SystemExit(f"esbuild {spec['version']} for {plat} is not pinned")

    binary = os.path.join(dest, ".esbuild", "esbuild")
    data = fetch(
        manifest["registry"],
        f"@esbuild/{plat}",
        spec["version"],
        spec["platforms"][plat],
        tarball_dir,
    )
    with tempfile.TemporaryDirectory() as work_dir:
        archive = os.path.join(work_dir, "esbuild.tgz")
        with open(archive, "wb") as f:
            f.write(data)
        with tarfile.open(archive) as tar:
            extract_member(tar, "package/bin/esbuild", dest, ".esbuild/esbuild")
    os.chmod(binary, os.stat(binary).st_mode | stat.S_IXUSR)
    return binary


def bundle(esbuild, tree_root, tree, entry, output):
    """Bundle an ES module tree into one file.

    esbuild names each module in a comment, by the path it reached it through,
    so the bundle is only reproducible if that path is.  Copying the tree to a
    scratch directory and running from there makes it "dist/esm/..." wherever
    the build system staged the inputs -- Bazel hands them over as symlinks into
    its repository cache, whose name would otherwise land in the output.
    """
    esbuild = os.path.abspath(esbuild)
    output = os.path.abspath(output)
    os.makedirs(os.path.dirname(output), exist_ok=True)
    with tempfile.TemporaryDirectory() as work_dir:
        shutil.copytree(
            os.path.join(tree_root, tree),
            os.path.join(work_dir, tree),
            symlinks=False,
        )
        subprocess.run(
            [
                esbuild,
                entry,
                "--bundle",
                "--format=esm",
                "--legal-comments=inline",
                f"--outfile={output}",
            ],
            cwd=work_dir,
            check=True,
        )


def bundle_from_entry(manifest, name, esbuild, entry_path, output):
    """Bundle a package whose tree is already extracted, given its entry file.

    The build systems know where the entry landed, not where the tree starts;
    the manifest says how deep it sits, which is what turns one into the other.
    """
    spec = manifest["packages"][name]["bundle"]
    entry = spec["entry"]
    entry_path = os.path.abspath(entry_path)
    suffix = os.sep + entry.replace("/", os.sep)
    if not entry_path.endswith(suffix):
        raise SystemExit(
            f"{entry_path} does not end in {entry}, so the tree root cannot be "
            "derived from it"
        )
    bundle(esbuild, entry_path[: -len(suffix)], spec["tree"], entry, output)


def populate(manifest, dest, tarball_dir):
    """Extract every package into dest and return served path -> file path."""
    assets = {}
    esbuild = None

    for name, spec in manifest["packages"].items():
        print(f"{name}@{spec['version']}", file=sys.stderr)
        data = fetch(
            manifest["registry"], name, spec["version"], spec["sha256"], tarball_dir
        )
        with tempfile.TemporaryDirectory() as work_dir:
            archive = os.path.join(work_dir, "package.tgz")
            with open(archive, "wb") as f:
                f.write(data)
            with tarfile.open(archive) as tar:
                for source, served in spec["files"].items():
                    path = extract_member(tar, "package/" + source, dest, served)
                    assets["/third-party/" + served] = path

                if "bundle" not in spec:
                    continue

                # The tree is only an input to the bundle, so it goes to a
                # scratch directory rather than next to what is served.
                tree_root = os.path.join(dest, ".trees", name)
                if os.path.exists(tree_root):
                    shutil.rmtree(tree_root)
                prefix = "package/" + spec["bundle"]["tree"] + "/"
                for member in tar.getmembers():
                    if member.isfile() and member.name.startswith(prefix):
                        extract_member(
                            tar,
                            member.name,
                            tree_root,
                            member.name[len("package/") :],
                        )

            if esbuild is None:
                esbuild = install_esbuild(manifest, dest, tarball_dir)
            output = os.path.join(dest, spec["bundle"]["output"])
            bundle(
                esbuild,
                tree_root,
                spec["bundle"]["tree"],
                spec["bundle"]["entry"],
                output,
            )
            assets["/third-party/" + spec["bundle"]["output"]] = output

    return assets


def stamp_value(manifest_bytes):
    """What has to be equal for an existing dest to be reusable."""
    return sha256_bytes(manifest_bytes + host_esbuild_platform().encode())


def emit_cmake(path, assets):
    lines = [
        "# Generated by fetch_packages.py -- do not edit.",
        "set(WEB_THIRD_PARTY_ASSET_ARGS",
    ]
    lines += [f'  "{served}={path}"' for served, path in sorted(assets.items())]
    lines += [")", "set(WEB_THIRD_PARTY_FILES"]
    lines += [f'  "{path}"' for _, path in sorted(assets.items())]
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
    parser.add_argument("--bundle", help="bundle this package's module tree")
    parser.add_argument("--esbuild", help="esbuild binary, with --bundle")
    parser.add_argument("--entry", help="the extracted entry point, with --bundle")
    parser.add_argument("--output", help="the bundle to write, with --bundle")
    parser.add_argument(
        "--print-sha256", help="print the digest of <package>@<version>"
    )
    args = parser.parse_args()

    manifest = read_manifest()

    if args.print_sha256:
        name, _, version = args.print_sha256.rpartition("@")
        url = tarball_url(manifest["registry"], name, version)
        with urllib.request.urlopen(url, timeout=120) as response:
            print(sha256_bytes(response.read()))
        return 0

    if args.bundle:
        if not (args.esbuild and args.entry and args.output):
            raise SystemExit("--bundle needs --esbuild, --entry and --output")
        bundle_from_entry(manifest, args.bundle, args.esbuild, args.entry, args.output)
        return 0

    if args.download_only:
        if not args.tarball_dir:
            raise SystemExit("--download-only needs --tarball-dir")
        for name, spec in manifest["packages"].items():
            fetch(
                manifest["registry"],
                name,
                spec["version"],
                spec["sha256"],
                args.tarball_dir,
            )
        plat = host_esbuild_platform()
        fetch(
            manifest["registry"],
            f"@esbuild/{plat}",
            manifest["esbuild"]["version"],
            manifest["esbuild"]["platforms"][plat],
            args.tarball_dir,
        )
        return 0

    if not args.dest:
        raise SystemExit("--dest is required")

    with open(MANIFEST_PATH, "rb") as f:
        stamp = stamp_value(f.read())
    stamp_path = os.path.join(args.dest, ".stamp")

    # CMake runs this on every configure; only a changed manifest is work.
    if os.path.exists(stamp_path):
        with open(stamp_path, encoding="utf-8") as f:
            if f.read().strip() == stamp:
                if args.emit_cmake and os.path.exists(args.emit_cmake):
                    return 0

    if os.path.exists(args.dest):
        shutil.rmtree(args.dest)
    os.makedirs(args.dest)
    assets = populate(manifest, args.dest, args.tarball_dir)

    if args.emit_cmake:
        emit_cmake(args.emit_cmake, assets)
    with open(stamp_path, "w", encoding="utf-8") as f:
        f.write(stamp + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
