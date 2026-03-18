"""
Verify tagging policy for test/orfs/ BUILD files.

Policy:
  - Test targets must have tags = ["orfs"] (no "manual")
  - Non-test targets must have tags = ["manual"] (no "orfs")

The "orfs" tag lets test_tag_filters control whether tests run:
  - Locally: test --test_tag_filters=-orfs skips them
  - CI: test:ci --test_tag_filters= includes them

The "manual" tag on non-test targets prevents bazelisk build //...
from building ORFS artifacts.

Call verify_orfs_tags() at the end of each BUILD file under test/orfs/.
See test/orfs/README.md for details.
"""

# Rule kinds that are tests
_TEST_KINDS = [
    "sh_test",
    "test_suite",
    # Rules from bazel-orfs and other external repos use
    # the _test suffix convention.
]

def _is_test_rule(kind):
    """Check if a rule kind is a test rule."""
    if kind in _TEST_KINDS:
        return True
    if kind.endswith("_test"):
        return True
    return False

def verify_orfs_tags():
    """Verify that all rules in the current package follow the tagging policy.

    Must be called at the end of a BUILD file, after all rules are defined.
    Fails the build if any rule violates the policy.
    """
    errors = []

    for name, rule in native.existing_rules().items():
        tags = rule.get("tags", [])
        kind = rule.get("kind", "")

        if _is_test_rule(kind):
            if "orfs" not in tags:
                errors.append("%s (%s): test target missing 'orfs' tag" % (name, kind))
            if "manual" in tags:
                errors.append("%s (%s): test target must not have 'manual' tag" % (name, kind))
        else:
            if "manual" not in tags:
                errors.append("%s (%s): non-test target missing 'manual' tag" % (name, kind))
            if "orfs" in tags:
                errors.append("%s (%s): non-test target should not have 'orfs' tag" % (name, kind))

    if errors:
        fail("Tagging policy violations in %s:\n  %s\n\nSee test/orfs/README.md for the tagging policy." % (
            native.package_name(),
            "\n  ".join(errors),
        ))
