#!/bin/bash

set -euo pipefail

isGitRepo="$(git -C . rev-parse 2>/dev/null; echo $?)"

if [[ "${isGitRepo}" != 0 ]]; then
    printf "git commit: unknown, this is not a git repository.\n(Please make sure that you have the latest code changes and add the commit hash in the description)\n"
else
    latestGitCommit="$(git ls-remote https://github.com/The-OpenROAD-Project/OpenROAD.git HEAD | awk '{print $1}')"
    currentGitCommit="$(git rev-parse HEAD)"
    if [[ ${currentGitCommit} != ${latestGitCommit} ]]; then
        echo "Please pull the latest changes and try again, if problem persists file a github issue with the re-producible test case."
        exit 1
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

echo "os: ${os} ${version}"
echo "$(cmake --version | sed 1q)"
echo "$(gcc --version | sed 1q)"
echo "$(clang --version | sed 1q)"