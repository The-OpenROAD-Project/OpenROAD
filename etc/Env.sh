#!/usr/bin/env bash

set -euo pipefail

# Make sure we are on the correct folder before beginning
cd "$(dirname $(readlink -f $0))/../"

mkdir -p build
exec > >(tee -i build/openroad-env.log)
exec 2>&1

isGitRepo="$(git -C . rev-parse 2>/dev/null; echo $?)"

if [[ "${isGitRepo}" != 0 ]]; then
    cat <<EOF
Unknown git commit, this is not a git repository.

Please make sure that you have the latest code changes and add the commit
hash in the description.

EOF
else
    latestGitCommit="$(git ls-remote https://github.com/The-OpenROAD-Project/OpenROAD.git HEAD | awk '{print $1}')"
    currentGitCommit="$(git rev-parse HEAD)"
    if [[ ${currentGitCommit} != ${latestGitCommit} ]]; then
        echo "[WARNING] Your current OpenROAD version is outdated."
        echo "It is recommened to pull the latest changes."
        echo "If problem persists, file a github issue with the re-producible test case."
    fi
    echo "Git commit: ${currentGitCommit}"
    echo "OpenSTA commit: $(git -C src/sta rev-parse HEAD 2>/dev/null || echo unknown)"
    # The commit pins the toolchain and all dependencies (MODULE.bazel.lock),
    # so local modifications are what remains to report.
    localChanges="$(git status --short --ignore-submodules=none)"
    if [[ -n "${localChanges}" ]]; then
        echo "Local changes:"
        echo "${localChanges}"
    fi
fi

platform="$(uname -s)"
echo "kernel: ${platform} $(uname -r) $(uname -m)"

case "${platform}" in
    "Linux" )
        if [[ -f /etc/os-release ]]; then
            os=$(awk -F= '/^NAME/{print $2}' /etc/os-release | sed 's/"//g')
            version=$(awk -F= '/^VERSION=/{print $2}' /etc/os-release | sed 's/"//g')
        else
            os="Unidentified OS, could not find /etc/os-release."
            version=""
        fi
        ;;
    "Darwin" )
        os="$(sw_vers | sed -n 's/^ProductName://p')"
        version="$(sw_vers | sed -n 's/^ProductVersion://p')"
        ;;
    *)
        os="Unidentified OS"
        version=""
        ;;
esac

echo "os: ${os} ${version}" | sed 's/\s\+/ /g'

if ! command -v bazelisk > /dev/null; then
    echo "bazelisk not found in PATH."
    exit 1
fi
bazelisk version | sed -n 's/^\(Bazelisk version\|Build label\):/\1:/p'

# Report bazelrc options from every source except the checked-in .bazelrc,
# which the commit already pins (e.g. user.bazelrc, ~/.bazelrc,
# /etc/bazel.bazelrc).
echo "bazelrc options outside the checked-in .bazelrc:"
bazelisk build --announce_rc --nobuild //:openroad 2>&1 \
    | awk -v repoRc="$(pwd)/.bazelrc:" '
        /^INFO: (Reading|Options provided)/ { show = (index($0, repoRc) == 0) }
        /^INFO: (Reading|Options provided)/ { if (show) print; next }
        /^  / { if (show) print; next }
        { show = 0 }
    '
