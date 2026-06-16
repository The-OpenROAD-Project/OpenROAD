"""Check that a Bazel target has the expected visibility in BUILD.bazel."""

import argparse
import re
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-file", required=True)
    parser.add_argument("--rule-type", required=True)
    parser.add_argument("--target", required=True)
    parser.add_argument("--expect", required=True, choices=["public", "not_public"])
    args = parser.parse_args()

    text = open(args.build_file).read()
    pattern = re.compile(
        rf'{args.rule_type}\(\s*name\s*=\s*"{args.target}".*?'
        rf"visibility\s*=\s*\[([^\]]*)\]",
        re.DOTALL,
    )
    m = pattern.search(text)
    if not m:
        print(f"ERROR: {args.rule_type} {args.target} not found")
        sys.exit(1)

    is_public = "//visibility:public" in m.group(1)
    if args.expect == "public" and not is_public:
        print(f"FAIL: {args.target} should be public")
        sys.exit(1)
    if args.expect == "not_public" and is_public:
        print(f"FAIL: {args.target} should not be public")
        sys.exit(1)
    print(f"PASS: {args.target} visibility is {args.expect}")


if __name__ == "__main__":
    main()
