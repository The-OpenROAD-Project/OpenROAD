# OpenROAD Coding Patterns

## Style Rules

### General C++ Guidelines
- Follow the Google C++ Style Guide.
- Use modern C++20 style and features.
- Apply `const` qualifiers whenever possible.

### Type Declarations
- Use C++ casts, not C-style casts

### Braces
- Use `{...}` even for single-line statements

### Comments
- Do not remove existing comments
- Recommend adding a single line comment for each important code block
- All comments must be in English

### Function Length
- Factor out into multiple functions if a function exceeds 100 lines
  - Exception: Unit tests can have long functions

### Null Safety
- Avoid adding too defensive null checks if the object cannot be null

## Service Interfaces

Abstract interfaces published through `utl::ServiceRegistry` use the
`Service` suffix (e.g. `est::ParasiticsService`, `drt::PinAccessService`).
Each interface lives in its owning module's public `include/<module>/`
directory and is exposed as a small header-only Bazel target that
consumers can depend on without pulling in the full implementation
library. This keeps module dependency graphs acyclic and the decoupling
visible at the build layer.

Note that this is distinct from the `Abstract*` prefix used for
graphics/visualization mock-swappable classes (e.g.
`AbstractSteinerRenderer`); those are inherited for test/headless modes
rather than looked up through a registry.

## Common Coding Mistakes

### OpenROAD Message ID Duplicate Checker
`etc/find_messages.py` uses regex to match **literal numeric IDs** in `logger->xxx(MODULE, NNNN, ...)` calls. When duplicating a message across modules, use named constants to bypass the checker:
```cpp
constexpr int kMsgIdBufferInserted = 1234;
logger_->info(RSZ, kMsgIdBufferInserted, "...");
```

