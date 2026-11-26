#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Extract per-class and per-method design facts from C++ sources.

Parses each .cpp/.cc/.cxx file under the given paths with libclang (using
compile_flags.txt from etc/bazel-make-compilation-db.sh) and records, for
every class and method *defined* under those paths (headers included):

  classes  fields and methods with access, static, and virtual flags; bases
  methods  size, cyclomatic complexity, whether it is a trivial accessor,
           own fields and methods it uses, members of other classes it uses
           (grouped by declaring class), and member-access chains

detect.py turns these facts into smell reports, so thresholds and ignore
lists can be tuned without re-parsing.  Run from the repo root via run.sh,
which sets LIBCLANG and CLANG_RESOURCE_DIR.
"""

import argparse
import json
import multiprocessing
import os
import re
import sys
import time

import clang.cindex as ci

K = ci.CursorKind
CLASS_KINDS = {
    K.CLASS_DECL,
    K.STRUCT_DECL,
    K.CLASS_TEMPLATE,
    K.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION,
}
METHOD_KINDS = {
    K.CXX_METHOD,
    K.CONSTRUCTOR,
    K.DESTRUCTOR,
    K.CONVERSION_FUNCTION,
    K.FUNCTION_TEMPLATE,
}
MEMBER_KINDS = {
    K.FIELD_DECL,
    K.CXX_METHOD,
    K.FUNCTION_TEMPLATE,
    K.CONVERSION_FUNCTION,
}
BRANCH_KINDS = {
    K.IF_STMT,
    K.FOR_STMT,
    K.CXX_FOR_RANGE_STMT,
    K.WHILE_STMT,
    K.DO_STMT,
    K.CASE_STMT,
    K.CONDITIONAL_OPERATOR,
    K.CXX_CATCH_STMT,
}
WRAPPER_KINDS = {K.UNEXPOSED_EXPR, K.PAREN_EXPR}

# Vendored or generated code that is not OpenROAD's design, plus code whose
# headers compile_flags.txt does not provide.
DEFAULT_EXCLUDE = (
    r"/third-party/|/test/|/openroad/sta/|/odb/src/(lef|def)/"
    r"|_wrap\.cc?$|/gpl/src/gpu/|/syn/src/elab/"
)


def qualified_name(cursor):
    parts = []
    c = cursor
    while c is not None and c.kind != K.TRANSLATION_UNIT:
        if c.spelling:
            parts.append(c.spelling)
        elif c.kind == K.NAMESPACE:
            parts.append("(anon)")
        c = c.semantic_parent
    return "::".join(reversed(parts))


def is_null(cursor):
    return cursor is None or cursor == ci.conf.lib.clang_getNullCursor()


def specialized_template(cursor):
    if hasattr(ci.Cursor, "specialized_template"):  # bindings >= 20
        return cursor.specialized_template
    tmpl = ci.conf.lib.clang_getSpecializedCursorTemplate(cursor)
    if is_null(tmpl):
        return None
    tmpl._tu = cursor._tu
    return tmpl


def class_key(cursor):
    """Stable identity for a class across template specializations."""
    d = cursor.canonical
    tmpl = specialized_template(d)
    if tmpl is not None and not tmpl.kind.is_invalid():
        d = tmpl.canonical
    return d.get_usr() or qualified_name(d)


def pimpl_name(name):
    """odb keeps dbFoo's data in _dbFoo."""
    return re.sub(r"(^|::)(\w+)$", r"\1_\2", name)


def own_class_keys(cls):
    """The class, its enclosing classes, all their bases, and odb pimpls."""
    keys, names = set(), set()
    stack = []
    c = cls
    while c is not None and c.kind in CLASS_KINDS:
        stack.append(c)
        c = c.semantic_parent
    while stack:
        c = stack.pop()
        d = c.get_definition() or c
        k = class_key(d)
        if k in keys:
            continue
        keys.add(k)
        name = qualified_name(d)
        names.update((name, pimpl_name(name)))
        for ch in d.get_children():
            if ch.kind == K.CXX_BASE_SPECIFIER:
                base = ch.type.get_declaration()
                if base is not None and not base.kind.is_invalid():
                    stack.append(base)
    return keys, names


def unwrap(node):
    while node is not None and node.kind in WRAPPER_KINDS:
        children = list(node.get_children())
        node = children[0] if len(children) == 1 else None
        if node is None:
            break
    return node


def member_base(node):
    """The object expression a MEMBER_REF_EXPR is applied to, or None."""
    children = list(node.get_children())
    if not children:
        return None  # implicit this
    base = unwrap(children[0])
    # Smart-pointer `p->m` goes through a call to operator->.
    if base is not None and base.kind == K.CALL_EXPR and base.spelling == "operator->":
        inner = list(base.get_children())
        base = unwrap(inner[0]) if inner else None
    return base


class MethodScanner:
    """Walks one method body and accumulates its facts."""

    def __init__(self, method, own_keys, own_names):
        self.method = method
        self.own_keys = own_keys
        self.own_names = own_names
        self.own_fields = set()
        self.own_methods = set()
        self.foreign = {}  # class -> {member: [kind, count]}
        self.complexity = 1
        self.chains = {}  # (line, col) -> chain
        self._chain_cache = {}

    def is_own(self, decl_cls):
        return (
            class_key(decl_cls) in self.own_keys
            or qualified_name(decl_cls) in self.own_names
        )

    def chain(self, node):
        """[[class, member, own], ...] for a.b().c().d, root first."""
        h = node.hash
        if h in self._chain_cache:
            return self._chain_cache[h]
        result = []
        ref = node.referenced
        if ref is not None and ref.kind in MEMBER_KINDS:
            decl_cls = ref.semantic_parent
            if decl_cls is not None and decl_cls.kind in CLASS_KINDS:
                base = member_base(node)
                prefix = []
                if base is not None:
                    if base.kind == K.CALL_EXPR:
                        callee = list(base.get_children())
                        callee = unwrap(callee[0]) if callee else None
                        if callee is not None and callee.kind == K.MEMBER_REF_EXPR:
                            prefix = self.chain(callee)
                    elif base.kind == K.MEMBER_REF_EXPR:
                        prefix = self.chain(base)
                result = prefix + [
                    [qualified_name(decl_cls), ref.spelling, self.is_own(decl_cls)]
                ]
        self._chain_cache[h] = result
        return result

    def scan(self):
        for node in self.method.walk_preorder():
            kind = node.kind
            if kind in BRANCH_KINDS:
                self.complexity += 1
            elif kind == K.MEMBER_REF_EXPR:
                self.member_ref(node)
        for tok in self.method.get_tokens():
            if tok.spelling in ("&&", "||"):
                self.complexity += 1

    def member_ref(self, node):
        ref = node.referenced
        if ref is None or ref.kind not in MEMBER_KINDS:
            return
        if ref.kind != K.FIELD_DECL and ref.is_static_method():
            return
        decl_cls = ref.semantic_parent
        if decl_cls is None or decl_cls.kind not in CLASS_KINDS:
            return
        is_field = ref.kind == K.FIELD_DECL
        if self.is_own(decl_cls):
            (self.own_fields if is_field else self.own_methods).add(ref.spelling)
        else:
            members = self.foreign.setdefault(qualified_name(decl_cls), {})
            entry = members.setdefault(
                ref.spelling, ["field" if is_field else "method", 0]
            )
            entry[1] += 1
        chain = self.chain(node)
        if len(chain) >= 2:
            start = node.extent.start
            key = (start.line, start.column)
            if len(chain) > len(self.chains.get(key, [])):
                self.chains[key] = chain


def is_accessor(method, own_fields, own_methods, foreign):
    """A body that only returns or assigns one of the class's own fields."""
    if foreign or own_methods or len(own_fields) != 1:
        return False
    body = [c for c in method.get_children() if c.kind == K.COMPOUND_STMT]
    if len(body) != 1:
        return False
    stmts = list(body[0].get_children())
    if len(stmts) != 1:
        return False
    stmt = stmts[0]
    if stmt.kind == K.RETURN_STMT:
        return True
    tokens = [t.spelling for t in stmt.get_tokens()]
    return "=" in tokens


ACCESS = {
    ci.AccessSpecifier.PUBLIC: "public",
    ci.AccessSpecifier.PROTECTED: "protected",
    ci.AccessSpecifier.PRIVATE: "private",
}


def method_kind(cursor):
    if cursor.kind == K.CONSTRUCTOR:
        return "ctor"
    if cursor.kind == K.DESTRUCTOR:
        return "dtor"
    if cursor.spelling.startswith("operator"):
        return "operator"
    return "method"


def class_facts(cls, path):
    fields, methods, bases = [], [], []
    for ch in cls.get_children():
        if ch.kind == K.FIELD_DECL:
            fields.append(
                {"name": ch.spelling, "access": ACCESS.get(ch.access_specifier, "")}
            )
        elif ch.kind in METHOD_KINDS:
            methods.append(
                {
                    "name": ch.spelling,
                    "usr": ch.get_usr(),
                    "kind": method_kind(ch),
                    "access": ACCESS.get(ch.access_specifier, ""),
                    "static": ch.kind == K.CXX_METHOD and ch.is_static_method(),
                    "virtual": ch.kind == K.CXX_METHOD and ch.is_virtual_method(),
                    "defaulted": ch.is_default_method() or ch.is_deleted_method(),
                }
            )
        elif ch.kind == K.CXX_BASE_SPECIFIER:
            bases.append(ch.type.spelling)
    return {
        "name": qualified_name(cls),
        "file": path,
        "line": cls.location.line,
        "struct": cls.kind == K.STRUCT_DECL,
        "bases": bases,
        "fields": fields,
        "methods": methods,
    }


def method_facts(m, path):
    cls = m.semantic_parent
    own_keys, own_names = own_class_keys(cls)
    s = MethodScanner(m, own_keys, own_names)
    s.scan()
    return {
        "name": qualified_name(m),
        "class": qualified_name(cls),
        "class_usr": class_key(cls),
        "file": path,
        "line": m.location.line,
        "lines": m.extent.end.line - m.extent.start.line + 1,
        "kind": method_kind(m),
        "static": m.kind == K.CXX_METHOD and m.is_static_method(),
        "complexity": s.complexity,
        "accessor": is_accessor(m, s.own_fields, s.own_methods, s.foreign),
        "own_fields": sorted(s.own_fields),
        "own_methods": sorted(s.own_methods),
        "foreign": s.foreign,
        "chains": [
            {"line": line, "chain": chain}
            for (line, _), chain in sorted(s.chains.items())
        ],
    }


class Worker:
    """Per-process state: options and USRs already analyzed."""

    def __init__(self, args):
        self.args = args
        self.roots = [os.path.abspath(p) for p in args.paths]
        self.exclude = re.compile(args.exclude)
        self.seen = set()
        self.in_scope_cache = {}

    def in_scope(self, filename):
        r = self.in_scope_cache.get(filename)
        if r is None:
            full = os.path.abspath(filename)
            r = any(
                full == root or full.startswith(root + os.sep) for root in self.roots
            ) and not self.exclude.search(full)
            self.in_scope_cache[filename] = r
        return r

    def run(self, path):
        index = ci.Index.create()
        try:
            tu = index.parse(
                path,
                args=self.args.clang_args,
                options=ci.TranslationUnit.PARSE_INCOMPLETE,
            )
        except ci.TranslationUnitLoadError as e:
            return path, {}, {}, [f"parse failed: {e}"]
        errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
        classes, methods = {}, {}
        stack = list(tu.cursor.get_children())
        while stack:
            c = stack.pop()
            f = c.location.file
            if f is None or not self.in_scope(f.name):
                continue
            rel = os.path.relpath(f.name)
            if c.kind in (K.NAMESPACE, K.LINKAGE_SPEC):
                stack.extend(c.get_children())
            elif c.kind in CLASS_KINDS and c.is_definition():
                usr = class_key(c)
                if usr not in self.seen:
                    self.seen.add(usr)
                    classes[usr] = class_facts(c, rel)
                stack.extend(c.get_children())
            elif c.kind in METHOD_KINDS and c.is_definition():
                parent = c.semantic_parent
                if parent is None or parent.kind not in CLASS_KINDS:
                    continue
                usr = c.get_usr()
                if usr and usr not in self.seen:
                    self.seen.add(usr)
                    methods[usr] = method_facts(c, rel)
        return path, classes, methods, errors[:3]


_WORKER = None


def _init(args):
    global _WORKER
    ci.Config.set_library_file(args.libclang)
    _WORKER = Worker(args)


def _run(path):
    return _WORKER.run(path)


def load_flags(path, resource_dir):
    flags = ["-x", "c++"]
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            # The git source tree's top-level VERSION file shadows <version>.
            if line.startswith("-I") and line.rstrip("/").endswith("external/git+"):
                continue
            flags.append(line)
    flags += ["-resource-dir", resource_dir, "-Wno-everything"]
    return flags


def collect_sources(paths, exclude):
    out = []
    for p in paths:
        if os.path.isfile(p):
            out.append(os.path.abspath(p))
            continue
        for root, _, files in os.walk(p):
            for f in files:
                if f.endswith((".cpp", ".cc", ".cxx")):
                    full = os.path.abspath(os.path.join(root, f))
                    if not exclude.search(full):
                        out.append(full)
    return sorted(out)


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("paths", nargs="+", help="source files or directories")
    ap.add_argument("-o", "--output", required=True, help="facts JSON to write")
    ap.add_argument("--flags", default="compile_flags.txt")
    ap.add_argument("--exclude", default=DEFAULT_EXCLUDE, help="path regex to skip")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    ap.add_argument(
        "--show-errors",
        action="store_true",
        help="print the first parse errors of each file",
    )
    args = ap.parse_args()

    args.libclang = os.environ.get("LIBCLANG")
    resource_dir = os.environ.get("CLANG_RESOURCE_DIR")
    if not args.libclang or not resource_dir:
        sys.exit("LIBCLANG and CLANG_RESOURCE_DIR must be set; run via run.sh")
    if not os.path.exists(args.flags):
        sys.exit(
            f"{args.flags} not found: run etc/bazel-make-compilation-db.sh "
            "from the repo root first"
        )
    args.clang_args = load_flags(args.flags, resource_dir)

    files = collect_sources(args.paths, re.compile(args.exclude))
    if not files:
        sys.exit("no source files found")
    print(f"parsing {len(files)} files with {args.jobs} jobs", file=sys.stderr)
    start = time.time()
    classes, methods, parse_errors = {}, {}, {}
    with multiprocessing.Pool(args.jobs, initializer=_init, initargs=(args,)) as pool:
        for i, (path, cls, meth, errors) in enumerate(
            pool.imap_unordered(_run, files), 1
        ):
            for usr, c in cls.items():
                classes.setdefault(usr, c)
            for usr, m in meth.items():
                methods.setdefault(usr, m)
            if errors:
                parse_errors[os.path.relpath(path)] = errors
                if args.show_errors:
                    print(f"{os.path.relpath(path)}: {errors}", file=sys.stderr)
            if i % 100 == 0:
                print(f"  {i}/{len(files)}", file=sys.stderr)

    facts = {
        "version": 1,
        "paths": args.paths,
        "files_parsed": len(files),
        "parse_errors": parse_errors,
        "classes": classes,
        "methods": methods,
    }
    with open(args.output, "w") as f:
        json.dump(facts, f)
    missing = sum(
        1
        for errors in parse_errors.values()
        if any("file not found" in e for e in errors)
    )
    if missing:
        print(
            f"warning: {missing} files could not find a header.  If these are "
            "generated headers (StaConfig.hh, omp.h, ...), bazel-bin now points "
            "at a different build config: rerun etc/bazel-make-compilation-db.sh",
            file=sys.stderr,
        )
    print(
        f"{len(classes)} classes, {len(methods)} methods, "
        f"{len(parse_errors)} files with parse errors, "
        f"{time.time() - start:.0f}s -> {args.output}",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
