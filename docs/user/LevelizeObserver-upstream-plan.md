# Proposed OpenSTA upstream changes — `LevelizeObserver` + `Sta::setLevelizeObserver`

## Status

Proposed. Not yet submitted upstream.

- OpenSTA branch staged at: `parallaxsw/OpenSTA` fork
- OpenROAD dependency: blocked until upstream lands.

## Motivation

OpenROAD's `dbSta` needs to attach its own `LevelizeObserver` so the
driver-vertex cache (`dbSta::levelizedDrvrVertices()`) is invalidated when
the timing graph levels change. Today this requires three workarounds:

1. **`OPENSTA_HOME` private include path.** `LevelizeObserver` is declared in
   `sta/search/Levelize.hh`, which is not part of OpenSTA's public include.
   `src/dbSta/src/CMakeLists.txt` adds `${OPENSTA_HOME}` and
   `${OPENSTA_HOME}/include/sta` as `PRIVATE` includes on `dbSta_lib` to
   reach it.
2. **Reaching into `Sta::levelize_`.** `dbSta::makeObservers` calls
   `levelize_->setObserver(...)` directly. `levelize_` is `protected` on
   `StaState`, so it works, but it is a private contract.
3. **Replicating `StaLevelizeObserver` behavior.**
   `Levelize::setObserver` overwrites and deletes any prior observer (single
   slot, no chain). The observer that `Sta::makeObservers` installs —
   `StaLevelizeObserver` — forwards level-change events to `Search` and
   `GraphDelayCalc` so their incremental-update iterators stay consistent.
   Because `dbSta` must replace that observer, it has to duplicate the
   forwarding by hand inside `DbStaLevelizeObserver`. If upstream ever adds
   work to `StaLevelizeObserver`, the dbSta copy silently drifts.

Reviewer feedback (gemini) on the OpenROAD PR called out both points:

> The `dbSta::makeObservers` override replaces the `StaLevelizeObserver`
> created by the base class. ... it would be safer to verify if OpenSTA
> could be updated to support observer chaining or multiple observers.

> It would make more sense for this observer to inherit from the existing
> observer and then call the parent class function instead of duplicating
> the contents.

> I think it makes more sense to add a set function for the levelize
> observer on the `Sta` class than to change the include search path to get
> access to `Levelize.hh`.

## Proposed OpenSTA changes

Five small changes, ~12 LOC of behavior. No functional change to existing
flows.

### 1. New public header `include/sta/LevelizeObserver.hh`

Holds both observer classes that previously lived in
`search/Levelize.hh` and `search/Sta.cc`.

```cpp
namespace sta {

class Vertex;
class Search;
class GraphDelayCalc;

class LevelizeObserver {
public:
  virtual ~LevelizeObserver() = default;
  virtual void levelsChangedBefore() = 0;
  virtual void levelChangedBefore(Vertex *vertex) = 0;
};

class StaLevelizeObserver : public LevelizeObserver {
public:
  StaLevelizeObserver(Search *search, GraphDelayCalc *graph_delay_calc);
  void levelsChangedBefore() override;
  void levelChangedBefore(Vertex *vertex) override;
private:
  Search *search_;
  GraphDelayCalc *graph_delay_calc_;
};

} // namespace sta
```

### 2. `search/Levelize.hh`

Drop the inline `LevelizeObserver` class body. Include the new public
header.

```diff
+#include "sta/LevelizeObserver.hh"
 ...
-class LevelizeObserver
-{ ... };
```

### 3. `search/Sta.cc`

Drop the file-local `StaLevelizeObserver` class body. Keep the function
definitions (they now define the public class).

### 4. `include/sta/Sta.hh`

Add public setter.

```diff
+class LevelizeObserver;
 ...
+  // Replace the Levelize observer. Takes ownership; deletes any prior
+  // observer. Subclass StaLevelizeObserver to extend the default behavior
+  // (Search + GraphDelayCalc forwarding) without re-implementing it.
+  void setLevelizeObserver(LevelizeObserver *observer);
```

### 5. `search/Sta.cc`

One-line implementation.

```cpp
void Sta::setLevelizeObserver(LevelizeObserver *observer) {
  levelize_->setObserver(observer);
}
```

CMake / Bazel pick up the new header automatically:
- CMake `install(DIRECTORY include/sta DESTINATION include)`.
- Bazel `opensta_lib` already globs `include/sta/*.hh`.

## OpenROAD follow-up after upstream lands

### `src/dbSta/src/dbSta.cc`

Replace the current observer + override with the cleaner version that
inherits from `StaLevelizeObserver` and uses `Sta::setLevelizeObserver`:

```cpp
#include "sta/LevelizeObserver.hh"   // public — no OPENSTA_HOME hack

class DbStaLevelizeObserver : public StaLevelizeObserver
{
 public:
  DbStaLevelizeObserver(dbSta* sta, Search* s, GraphDelayCalc* gdc)
      : StaLevelizeObserver(s, gdc), sta_(sta) {}

  void levelsChangedBefore() override {
    StaLevelizeObserver::levelsChangedBefore();   // parent: Search + GDC
    sta_->invalidateLevelizedDrvrVertices();
  }
  void levelChangedBefore(Vertex* v) override {
    StaLevelizeObserver::levelChangedBefore(v);
    sta_->invalidateLevelizedDrvrVertices();
  }

 private:
  dbSta* sta_;
};

void dbSta::makeObservers() {
  Sta::makeObservers();
  setLevelizeObserver(
      new DbStaLevelizeObserver(this, search_, graph_delay_calc_));
}
```

Differences from the current implementation:

- `#include "sta/LevelizeObserver.hh"` instead of
  `#include "search/Levelize.hh"`.
- `DbStaLevelizeObserver` inherits from `StaLevelizeObserver` (public class)
  and calls `StaLevelizeObserver::levelsChangedBefore()` /
  `levelChangedBefore()` instead of hand-replicating the Search +
  GraphDelayCalc forwarding.
- `setLevelizeObserver(...)` replaces the previous
  `levelize_->setObserver(...)` reach-in.

### `src/dbSta/src/CMakeLists.txt`

Drop the private include hack:

```diff
 target_include_directories(dbSta_lib
   PUBLIC
     ../include
     ${PROJECT_SOURCE_DIR}/include
-  PRIVATE
-    # Needed for search/Levelize.hh and the unprefixed transitive
-    # OpenSTA headers it includes (e.g. Graph.hh) which are not part of
-    # OpenSTA's public include.
-    ${OPENSTA_HOME}
-    ${OPENSTA_HOME}/include/sta
 )
```

`dbSta_lib`'s public dependency on the `OpenSTA` CMake target already
exposes `include/`, and Bazel's `opensta_lib` `hdrs` already lists the new
header — both pick up `sta/LevelizeObserver.hh` without extra wiring.

### What does **not** change

- `dbSta::levelizedDrvrVertices()` body, the cache invariant, the
  `bool drvr_vertices_level_valid_` flag, and the `clear()` inside
  `invalidateLevelizedDrvrVertices()` are all independent of the upstream
  change. The `clear()` is still required because the cache vector must be
  empty when the flag is `false` — otherwise the next rebuild's
  `push_back` appends to stale (and possibly dangling) contents. See
  `dbSta::invalidateLevelizedDrvrVertices()` doc.
- `dbStaCbk::inDbInstCreate` / `inDbInstDestroy` calls to
  `invalidateLevelizedDrvrVertices()` remain — they plug the gap where
  `Sta::deleteLeafInstanceBefore` bypasses the `LevelizeObserver` path.
- Existing rsz call sites (`Resizer.cc`, `RepairDesign.cc`, `Rebuffer.cc`)
  are untouched.
- Regression test `src/dbSta/test/levelize_drvr_vertices1.tcl` and its
  golden remain valid.

## Why this is worth it

| Concern | Today | After upstream |
|---|---|---|
| `OPENSTA_HOME` PRIVATE include | required | removed |
| Reach into `Sta::levelize_` | yes | no |
| Replicate `StaLevelizeObserver` forwarding | yes (drift risk) | no (inherit) |
| Upstream adds new work to `StaLevelizeObserver` | silently lost in dbSta | inherited automatically |


