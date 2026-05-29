# mock-readline

A clean-room, BSD-3-Clause stub that overrides the `readline` Bazel module
(`local_path_override` in the root `MODULE.bazel`).

## Why

`abc` and `yosys` link GNU Readline only for their interactive REPLs. OpenROAD
drives both in batch mode (scripts / `-c`), never the REPL, so those symbols are
linked but never called.

Pulling real readline is undesirable for two reasons:

1. **License.** GNU Readline is GPL; this project is BSD-3-Clause. The stub
   contains no readline source -- only no-op definitions of the public API
   surface that `abc`/`yosys` reference (function/symbol names are interface
   facts, not copied expression).
2. **Build.** Real `readline` drags in `ncurses -> sed`, and `sed`'s bundled
   gnulib `memchr.c` fails to compile against glibc 2.43+ (its `_Generic`
   `memchr` macro). Stubbing readline removes `ncurses` and `sed` from the
   graph entirely.

This works in tandem with `abc@0.64-yosyshq.bcr.2`, whose overlay routes abc
through `readline` instead of depending on `ncurses` directly.

`readline()` returns `NULL` (EOF), so any REPL loop exits immediately. If a
build configuration ever genuinely needs interactive readline, drop the
override and depend on the real BCR module instead.
