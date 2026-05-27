# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""@slang -> @sv-lang//:libsvlang alias repo.

Standalone repo so the slang-elab submodule BUILD's bare @slang reference
resolves without modification. Drop once slang-elab moves to BCR and
references @sv-lang directly.
"""

_BUILD = """\
alias(
    name = "slang",
    actual = "@sv-lang//:libsvlang",
    visibility = ["//visibility:public"],
)
"""

def _slang_alias_repo_impl(rctx):
    rctx.file("BUILD.bazel", _BUILD)

_slang_alias_repo = repository_rule(implementation = _slang_alias_repo_impl)

def _slang_alias_impl(_module_ctx):
    _slang_alias_repo(name = "slang")

slang_alias = module_extension(implementation = _slang_alias_impl)
