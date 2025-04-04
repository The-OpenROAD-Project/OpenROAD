#!/usr/bin/env bash

set -x
cd "$(dirname $(readlink -f $0))/../"
set +e
eval "$2"
ret=$?
mkdir -p "$3"
mv build "$3"/build
mv src "$3"/src
set -e
exit $ret
