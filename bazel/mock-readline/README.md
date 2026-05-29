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

`readline()` returns `NULL` (EOF), so any REPL loop exits immediately.

## Scope

`local_path_override` only takes effect in the root module; Bazel ignores it
when OpenROAD is consumed as a dependency. So the stub applies *only* when the
OpenROAD repo is built directly (developers, CI) -- exactly the batch-only case
where it is safe. A downstream project that depends on OpenROAD is the root
module itself, this override is ignored, and `readline` resolves to the real
BCR module (GPL) as usual -- no action needed to get the real one. The GPL code
therefore never enters OpenROAD's own graph or distribution.

To use real readline while building the OpenROAD repo directly, drop the
override and depend on the real BCR module instead.
