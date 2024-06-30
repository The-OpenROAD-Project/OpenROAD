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
    else
        echo "Git commit: ${currentGitCommit}"
    fi
fi

platform="$(uname -s)"
echo "kernel: ${platform} $(uname -r)"

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
        echo "OS: ${os} ${version}"
        ;;
esac

echo "os: ${os} ${version}" | sed 's/\s\+/ /g'
cmake --version | sed 1q
envTempDir="$(mktemp -d)"
cmake -B "${envTempDir}" | sed -n '/--/p'
rm -rf "${envTempDir}"
