#!/bin/bash
function setpaths() {
    local DIR=$(readlink -f "$(dirname "${BASH_SOURCE[0]}")")
    export PATH=$DIR/dependencies/bin:$PATH
}

setpaths
