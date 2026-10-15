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

## Options Structs

A command with more than a few settings takes them as an options struct
declared once in C++ (`utl/Validate.h`). The struct's default member
initializers are the defaults; a static `schema()` names each field once with
its help, constraints and message id. That one declaration gives the Tcl
`-options` dict and the Tcl switches (`utl/OptionsArgs.h`: one per field, `-timing.nets_percentage` for
`timing.nets_percentage`, a flag for a bool, `-no_<field>` for a bool that is on by default, `-group.no_<field>` inside a group), the Python class (kwargs, dicts, attributes), the validation
(same message from C++, Python and Tcl), the `help` synopsis,
`describe_options`, `report_options` and the README table
(`describe_options -markdown`). See `gpl::PlaceOptions`
(`src/gpl/include/gpl/Replace.h`, schemas in `src/gpl/src/replace.cpp`),
`rsz::RepairDesignOptions` and `grt::GlobalRouteOptions`.

- **Adding an option**: one struct member with its default, one
  `.field("name", &S::name, <id>, "help", constraints...)` line (no id or
  constraints for a plain bool), and the README table regenerated. The Tcl
  switch, its help and the Python attribute follow; no `.i`, `.tcl` or
  Python change.
- **Embedding a sub-tool's options**: a member of the sub-tool's struct and a
  `.group("name", &S::member, "help")` line; the struct's initializer can
  override the sub-tool's defaults (`grt::GlobalRouteOptions g{.allow_congestion
  = true}`). Its fields become switches of the outer command
  (`-timing.repair_design.slew_margin`).
- **Converting a command**: struct + `schema()`; the C++ entry point takes
  the struct and calls `utl::validate(options, logger)`; the module init
  registers `utl::OptionsRegistry::get().add("command", S::schema(),
  aliases)` before its Tcl is evaluated, where `aliases` maps each old switch
  that differs from a generated name onto its dotted path; the Tcl proc keeps
  only the switches about how the command runs
  (`sta::parse_key_args ... keys {...} flags {...} 0`) and passes the rest of
  `$args` to a `%inline` function that calls `optionsFromArgs<S>(interp,
  args, "command")` (`src/options.i`); its `define_cmd_args` appends
  `[utl::options_synopsis_cmd command]`; the `-py.i` gets
  `%options_struct(ns::S, S)` (`src/options-py.i`).
- Names are snake_case, booleans are phrased positively, units are SI in
  C++ (tag a field `utl::Distance{}` etc. for the Tcl user-unit conversion),
  and a value that is "unset" is `std::optional`.
- Range checks belong in the schema, not in the Tcl proc or the algorithm;
  `utl::Validator` stays for one-off checks inside algorithms.

## Common Coding Mistakes

### OpenROAD Message ID Duplicate Checker
`etc/find_messages.py` uses regex to match **literal numeric IDs** in `logger->xxx(MODULE, NNNN, ...)` calls. When duplicating a message across modules, use named constants to bypass the checker:
```cpp
constexpr int kMsgIdBufferInserted = 1234;
logger_->info(RSZ, kMsgIdBufferInserted, "...");
```

