# linenoise (vendored)

Source: <https://github.com/antirez/linenoise> at tag `2.0`.
License: BSD-2-Clause (see `LICENSE`).

Used by OpenROAD's interactive CLI Tcl REPL
(`src/tcl_readline_setup.cc`) — replaces the previous tclreadline + GNU
readline stack (the latter was GPL).

## Local divergence from upstream

`linenoise.c` `completeLine()` has been patched to use **readline-style
list-display TAB completion** instead of upstream's cycle-through-candidates
behavior:

- **0 candidates** → beep.
- **1 candidate** → insert it.
- **N>1, common prefix advances the buffer** → advance buffer to the
  longest common prefix, no list yet.
- **N>1, no further common prefix** → print the list of candidates below
  the prompt, then redraw the prompt + current buffer.

This matches what users had with tclreadline.  Search the file for
`OpenROAD list-display` to locate the patched section.
