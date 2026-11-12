# OpenROAD Coding Patterns

## Style Rules

### General C++ Guidelines
- Follow the Google C++ Style Guide.
- Use modern C++20 style and features.
- Always use braces for `if`/`else`/`for`/`while` bodies, even single
  statements.
- In code you write or modify, apply `const` wherever possible: local
  variables, member functions, `const T&`/`const T*` parameters, and
  by-value parameters in function definitions (not in declarations).
- Data members may be `const` when the type doesn't need to be copy- or
  move-assignable.
- Don't return by `const` value; it prevents moves.
- Use C++ casts, not C-style casts.
- When a new cross-module dependency would otherwise add a dependency
  on another module's implementation library (or a cycle), depend on
  the interface and look up the provider in `utl::ServiceRegistry`
  instead of having an instance passed in: `find<I>()` returns null
  when no provider is registered, `require<I>()` errors. See
  `src/utl/include/utl/ServiceRegistry.h` for conventions.

### Comments and Prose
- Keep existing comments unless they're clearly outdated or wrong.
- Add a brief one-line comment above non-obvious logic, workarounds,
  or complex blocks. Don't comment self-explanatory code.
- Comments describe current behavior; history and rationale for a
  change go in the commit message or PR.
  - Exception: keep the reason when the code would otherwise look
    wrong or removable (a non-obvious gotcha, the bug that forced
    this design). State it as a current constraint ("X must precede Y
    because …"), not as a narrative.
- Write in English, using American spelling (e.g., "color", "behavior").
- Use the Oxford comma.

### Functions
- Keep new functions under ~100 lines, and don't push an existing
  function past that. Don't split already-long functions as a side
  effect of a small change. Unit tests are exempt.

### Debugging
- Trace bugs upstream: when a bug appears in output (e.g., Verilog),
  fix it where the data is created (e.g., `buffer_ports`,
  `remove_buffers`), not where it is serialized (e.g., `VerilogWriter`).

### Null Safety
- Don't add null checks for pointers that can't be null. Prefer
  references when null isn't a valid value (ODB objects are passed by
  pointer by convention).

## Common Coding Mistakes

### OpenROAD Message ID Duplicate Checker
`etc/find_messages.py` uses regex to match **literal numeric IDs** in
`logger->xxx(MODULE, NNNN, ...)` calls and flags any (MODULE, ID) pair
that appears more than once. Give each new message its own ID. Only when
one message is deliberately emitted from several call sites, define the ID
once as a named constant (the checker only counts literal IDs):
```cpp
constexpr int kMsgIdBufferInserted = 1234;
logger_->info(RSZ, kMsgIdBufferInserted, "...");
```
