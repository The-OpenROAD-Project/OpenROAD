#!/bin/bash
set -e
bazelisk run //:tidy_bzl
bazelisk run //:tidy_tcl
