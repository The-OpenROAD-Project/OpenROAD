# ODB Code Generator

This directory contains the code generator for OpenDB (`odb`) database objects.
A large fraction of `odb`'s C++ — the public classes in `include/odb/db.h`, the
private storage classes in `src/db/`, their serializers, comparators, memory
accounting, and the object iterators — is generated from JSON schema files
rather than written by hand. This document is for developers who maintain `odb`
or the generator itself; it is **not** an end-user guide.

## Why a generator

Every `odb` object follows the same boilerplate-heavy pattern:

- a **public** class `dbFoo` (in `include/odb/db.h`) exposing getters/setters,
- a **private** storage class `_dbFoo` (in `src/db/dbFoo.h`) holding the fields,
- stream `operator>>` / `operator<<` for disk serialization,
- `operator==` / `operator<` for diffing two databases,
- `collectMemInfo()` for heap accounting,
- constructor/destructor wiring for owned tables and `char*` strings,
- registration in the `dbObjectType` enum and its hash maps,
- one iterator class per parent→child table relationship.

Writing this by hand is repetitive and easy to get subtly wrong (e.g. a field
added to the struct but forgotten in the serializer, breaking on-disk
compatibility). The generator derives all of it from a per-class description of
the fields, so the only thing a developer writes by hand is the genuinely
custom logic, kept in clearly marked **User Code** blocks (see
[Section merging](#section-merging-user-code-vs-generated-code)).

## Quick start

```shell
# Regenerate every odb file in place (writes into ../db and ../../include/odb):
./generate          # wrapper for: python3 gen.py

# Keep the intermediate rendered files for inspection:
python3 gen.py --keep_generated   # leaves them in ./generated/

# Preserve empty User Code blocks (see "Adding to an empty section" below):
python3 gen.py --keep_empty
```

The generator depends only on `jinja2` (CI pins `jinja2==3.1.6`) and
`clang-format` (run on every generated C++ file). It must be run from this
directory.

> **CI invariant:** running `./generate` on a clean checkout must produce **no
> git diff**. The workflow `.github/workflows/github-actions-are-odb-files-generated.yml`
> fails the build otherwise. After editing any schema file, template, or the
> generator itself, re-run `./generate` and commit the regenerated C++ alongside
> your change.

## Pipeline overview

`gen.py` drives four stages:

```
load_schema()      read schema.json + every schema/**/*.json class file
        │
process_schema()   derive everything the templates need:
        │            - expand relations into parent/child fields
        │            - classify every field into a FieldType "kind"
        │            - resolve accessor names, includes, forward decls
        │            - pack bitfields into a flags_ struct
        │            - compute operator==/< component lists
        │            - assign each class a stable dbObjectType hash
        │
generate()         render Jinja templates into ./generated/
        │
_merge_files()     splice generated sections into the real source files,
                   preserving hand-written User Code, then clang-format
```

## Files in this directory

| File | Role |
|------|------|
| `gen.py` | Entry point and orchestration. Loads the schema, runs all the derivation passes (`process_schema`), renders templates, and merges output into the tree. |
| `schema_models.py` | Dataclasses (`Schema`, `Class`, `Field`, `Struct`, `Enum`, `Relation`, `Iterator`) that the JSON deserializes into. `from_dict` rejects unknown keys so JSON typos fail loudly. |
| `field_types.py` | The `FieldType` hierarchy — the single source of truth for how a field's C++ type is declared, serialized, compared, copied, and accessed. `make_field_type()` classifies a field into one kind. |
| `helper.py` | Pure helper functions: type-string predicates (`is_ref`, `is_hash_table`, `is_set_by_ref`, …), name munging (pluralize, camel-case, table name), template-argument parsing, and the FNV-1a hash for object types. |
| `parser.py` | The section merger. Knows how to find `Generator Code`/`User Code` regions in an existing file and splice new generated content in without losing hand-written code. |
| `schema.json` | Top-level schema: `classes_dir`, the list of `iterators`, the list of `relations`, and `extern_enums`. |
| `schema/**/*.json` | One file per database class (grouped into `chip/`, `tech/`, `gds/`, `scan/` subdirectories). |
| `templates/*.jinja` | The Jinja2 templates that emit C++ / CMake. |
| `test_*.py` | Unit tests (see [Testing](#testing)). |
| `generate` | Thin shell wrapper: `python3 gen.py`. |

## Schema format

### Top-level `schema.json`

```jsonc
{
  "classes_dir": "schema",          // directory scanned recursively for class files
  "extern_enums": ["dbGDSSTrans"],  // enum types defined outside the generator;
                                    //   treated as pass-by-value like other enums
  "iterators": [ ... ],             // see "Iterators"
  "relations": [ ... ]              // see "Relations"
}
```

### Per-class file (`schema/**/dbFoo.json`)

```jsonc
{
  "name": "dbAccessPoint",          // public class name; private class is _dbAccessPoint
  "type": "dbObject",               // optional base class (default "dbObject")
  "description": ["A line ...",     // optional doc-comment lines emitted above the
                  "another line"],  //   public class in db.h
  "fields": [ ... ],                // see "Fields"
  "structs": [ ... ],               // optional nested structs (public or private)
  "enums": [ ... ],                 // optional nested enums (public or private)
  "h_includes":   ["dbVector.h"],   // extra headers for the generated .h
  "cpp_includes": ["odb/dbTypes.h"],// extra headers for the generated .cpp
  "do_not_generate_compare": false, // skip operator==/< (also drops the std::less<> in dbCompare.inc)
  "hash": "0x1234ABCD",             // optional explicit object-type hash (default: FNV-1a of name)
  "ostream_scope": false,           // wrap operator<< in a named dbOStreamScope
  "assert_alignment_is_multiple_of": null  // emit a static_assert on alignof (pointer tagging)
}
```

The set of accepted keys is exactly the dataclass fields in
`schema_models.py`; an unknown key raises a `ValueError` naming the offending
key. When adding a feature, add the key to the dataclass first.

### Fields

A field describes one member of the private `_dbFoo` storage class and the
accessors generated for it. Common keys:

| Key | Meaning |
|-----|---------|
| `name` | C++ member name, conventionally trailing-underscore (`point_`). The setter argument and accessor names are derived from it. |
| `type` | C++ type string. May be a scalar, enum, `std::*`, `Point`/`Rect`/…, `dbId<_dbX>` (object reference), `dbVector<...>`, `dbHashTable<...>`, or the pseudo-type `"bit"` (→ `bool` 1-bit field). |
| `flags` | List of behavior flags — see [Flags](#flags-reference). |
| `bits` | Bitfield width. Fields with `bits` are packed into a generated `flags_` struct. |
| `default` | Initializer emitted in the constructor. |
| `parent` | For a `dbId<_dbX>` reference, the class that owns the table the id resolves against. Required for the getter to do a table lookup. |
| `comment` / `public_comment` | Comment emitted next to the member / next to the public getter. |
| `argument` | Override the setter's argument name (default: `name` without underscores). |
| `setterFunctionName` / `getterFunctionName` | Override the derived accessor names (e.g. `getBPin`). |
| `schema` | Guard serialization-in behind `isSchema(<token>)` for backward-compatible format upgrades. |
| `table` / `page_size` / `dbSetGetter` | Set automatically for relation-generated table fields; not usually written by hand. |

### Field kinds (`field_types.py`)

`make_field_type()` classifies each field's `type` into exactly one `FieldType`,
and the rest of the generator asks that object how to handle the field instead
of re-parsing the type string. Classification precedence:

| Kind | Matches | Accessor / behavior |
|------|---------|---------------------|
| `TableType` | a relation's owned `dbTable<_Child>*` | getter returns a `dbSet<Child>`; serialized/compared by dereference; constructed in the ctor, deleted in the dtor |
| `RefType` | `dbId<_X>` **with** a `parent` | stores an OID; getter resolves it via the parent's owner table; setter takes `X*` |
| `HashTableType` | `dbHashTable<_X>` | exposes a `findX(name)` getter; no setter |
| `BoolType` | a width-1 bitfield | accessor type is `bool` (`isFoo()` getter) |
| `VectorType` | `dbVector<...>` / `std::vector<...>` | getter is an out-parameter (`void getFoo(std::vector<...>&)`); exposed as `std::vector` |
| `CharPtrType` | `char*` | deep-copied; getter returns `const char*`; `name_` is freed in the dtor |
| `StringType` | `std::string` | scalar-like, but set by const ref |
| `PairType` | `std::pair<...>` | scalar-like, but set by const ref |
| `ScalarType` | everything else (PODs, enums, `std::` tail) | by-value getter/setter |

The `Field` properties the templates read (`setterArgumentType`,
`getterReturnType`, `member_decl`, `isSetByRef`, `refTable`, …) are all
delegated to the field's `kind`. To change how a category of type is handled,
edit the corresponding `FieldType` subclass rather than special-casing in the
templates.

### Flags reference

| Flag | Effect |
|------|--------|
| `private` | No getter and no setter (implies `no-get` + `no-set`). |
| `no-get` | Don't generate a getter. |
| `no-set` | Don't generate a setter. |
| `no-serial` | Exclude from `operator>>` / `operator<<`. |
| `no-cmp` | Exclude from `operator==`. |
| `cmpgt` | Include in `operator<` (the ordering comparison). |
| `no-meminfo` | Exclude from generated `collectMemInfo()` accounting (handle it in the User Code block — e.g. a vector of strings that needs element-level accounting). |
| `no-template` | Don't forward-declare the field's template arguments. |
| `no-destruct` | Don't free a `char*` field in the destructor. |

### Relations

A relation declares a 1-to-many ownership: a `parent` class owns a table of
`child` objects. From one entry the generator synthesizes, on the parent, an
owned `dbTable<_Child>*` field with a `dbSet<Child>` getter (and, for `hash`
relations, a `dbHashTable` plus a `next_entry_` link on the child).

```jsonc
{
  "parent": "dbTechLayer",
  "child":  "dbTechLayerCutClassRule",
  "type":   "1_n",                       // only 1_n is supported
  "tbl_name": "cut_class_rules_tbl_",     // the generated table member name
  "hash":   true,                         // also generate a name→child hash table
  "page_size": 1024,                      // optional dbTable page size
  "schema": "kSchemaFoo",                 // gate serialization for format upgrades
  "flags":  ["no-serial"],                // flags applied to the generated fields
  "create": true,                         // generate Child::create(Parent*)
  "destroy": true                         // generate Child::destroy(Child*)
}
```

`create`/`destroy` emit the canonical owned-table factory methods on the child.
Set them only when the hand-written `create`/`destroy` would do nothing more
than allocate/free a table entry (and free properties on destroy); anything
with extra logic must keep its hand-written versions in User Code.

### Iterators

Each iterator entry generates a `dbIterator` subclass that walks a parent's
table:

```jsonc
{
  "name": "dbModuleInstItr",
  "parentObject": "dbInst",         // element type the iterator yields
  "tableName": "inst_tbl",          // the table member it walks
  "reversible": "true",
  "orderReversed": "true",
  "sequential": 0,
  "customEnd": "true",              // emit a User Code end() instead of "return 0"
  "tablePageSize": 1024,            // must match the table's page size
  "flags": ["private"],
  "includes": ["dbModule.h"]        // extra headers for the .cpp
}
```

The `begin`, `next`, and (for `customEnd`) `end` bodies are left as User Code
blocks for you to fill in — only the boilerplate around them is generated.

## Generated output

`generate()` renders, then `_merge_files()` writes:

- **Per class:** `_dbFoo` private header and `.cpp` (`impl.h.jinja`,
  `impl.cpp.jinja`) → `src/db/dbFoo.{h,cpp}`. The `.cpp` contains the
  serializers (`serializer_in/out.cpp.jinja`), comparators, constructor,
  destructor, `collectMemInfo`, and accessor definitions.
- **Per iterator:** `dbFooItr.{h,cpp}` (`itr.h.jinja`, `itr.cpp.jinja`).
- **Shared, schema-wide files:**
  - `db.h.jinja` → public class declarations/definitions in `include/odb/db.h`.
  - `dbObject.h.jinja` / `dbObject.cpp.jinja` → the `dbObjectType` enum entries
    and the hash↔type maps.
  - `dbCompare.inc.jinja` → deleted `std::less<dbFoo*>` specializations.
  - `CMakeLists.txt.jinja` → the generated-source list in `src/CMakeLists.txt`.

Each class is assigned a stable `dbObjectType` hash (explicit `hash` key, else
FNV-1a of the name); a collision between two classes is a hard error.

## Section merging (User Code vs generated code)

Generated files are **not** wholesale-overwritten. Each managed file is divided
into regions by paired comment markers:

```cpp
// Generator Code Begin Cpp
   ... fully owned by the generator; replaced on every run ...
   // User Code Begin Constructor
      ... your hand-written code; preserved across runs ...
   // User Code End Constructor
// Generator Code End Cpp
```

`parser.py` (`Parser`) implements the merge:

1. `parse_user_code()` — capture the current contents of every `User Code` block
   from the existing file.
2. `clean_code()` — strip the old generator-owned regions.
3. `parse_source_code()` — read the freshly rendered generator regions.
4. `write_in_file()` — splice the saved User Code back into the matching blocks
   inside the new generated regions, then write the file.

Key consequences for developers:

- **Put custom logic only inside `User Code` blocks.** Anything outside them is
  overwritten on the next `./generate`.
- A `User Code` block whose name no longer exists in the regenerated output is a
  **hard error** ("User tags … not used") — the generator refuses to silently
  drop your code. If you rename/remove a field whose User Code you still need,
  move that code first.
- Empty `User Code` blocks are removed by default to keep files tidy. To add
  code to a section that is currently emitted as empty, regenerate with
  `--keep_empty`, fill in the block, then drop the flag and regenerate again.
- `CMakeLists.txt` uses `#`-style markers; everything else uses `//`.
- Files that don't yet exist are simply copied out; existing files are merged.

## Testing

```shell
python3 -m unittest test_schema_validation   # schema typing: unknown-key rejection, bool coercion
python3 -m unittest test_field_types          # every field in the real schema gets a valid kind
python3 -m unittest test_generator_snapshot   # pins rendered output for one field of each kind
```

`test_field_types.py` and `test_generator_snapshot.py` load the **committed**
schema and exercise `process_schema()` / template rendering, so they catch
regressions in the derivation logic. The whole-tree guarantee — that
`./generate` produces no diff — is enforced by CI, not these unit tests.

## Adding a new database class — checklist

1. Create `schema/<area>/dbFoo.json` with the class `name`, its `fields`, and
   any `structs`/`enums`/includes.
2. If the object is owned by another object, add a `relation` in `schema.json`
   (parent → `dbFoo`). Add `create`/`destroy` if the trivial factories suffice.
3. If clients iterate it, add an `iterator` entry in `schema.json`.
4. Run `./generate`.
5. Fill in the `User Code` blocks (e.g. iterator `begin`/`next`, custom
   `create`/`destroy`, any methods declared in the `…PublicMethods` block).
6. Re-run `./generate`, build, and run the tests. Commit the regenerated C++
   together with the schema change so CI stays green.
</content>
</invoke>
