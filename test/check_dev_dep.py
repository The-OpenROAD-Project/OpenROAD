"""Check that a bazel_dep in MODULE.bazel has dev_dependency = True."""

import argparse
import re
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--module-file", required=True)
    parser.add_argument("--dep", required=True)
    args = parser.parse_args()

    text = open(args.module_file).read()
    if not re.search(
        rf'name\s*=\s*"{args.dep}".*?dev_dependency\s*=\s*True',
        text,
    ):
        print(f"FAIL: {args.dep} should be dev_dependency")
        sys.exit(1)
    print(f"PASS: {args.dep} is dev_dependency")


if __name__ == "__main__":
    main()
