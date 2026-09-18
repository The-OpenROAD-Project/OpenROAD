# Direction of the .odb

## Statement

The `.odb` is the design's state of record. It carries what the design
owns and what a later stage needs to continue. Restoring an `.odb`
restores the design, constrained and ready to time, with no other file
required.

Text formats are how a human writes input. The `.odb` is how stages hand
the design to each other. A flow reads each text input once and from then
on the design travels as a database.

Single-concern pull requests that move one more piece of design state
into the `.odb` are welcome. A complete roadmap is not a precondition;
the rules at the end of this document are what make piecewise progress
safe.

## Why

The Developer Guide states the founding principle: one tool, one process,
one database, and file-based communication between tools is strongly
discouraged. Design state that travels between stages as side files is
the remaining exception to that principle.

The open literature agrees. OpenAccess, the Si2 open standard for an IC
design database, models constraints as objects owned by the design
(`oaConstraint`, `oaConstraintGroup`), not as files kept beside it.
IEEE 1801 defines power intent as design data with a text interchange
form. For anyone practised in the art it is the principle of least
astonishment: restoring a saved design is expected to restore a
constrained, timeable design.

## What is in the .odb today

| state | text input | in the `.odb` as |
|---|---|---|
| technology, cells, netlist, placement, routing | LEF, DEF, Verilog | the core data model |
| power intent | UPF | `dbPowerDomain`, `dbIsolation`, `dbLevelShifter`, `dbPowerSwitch` |
| scan chains | scan insertion commands | `dbScanChain`, `dbScanPin` |
| global connect rules | `global_connect` | `dbGlobalConnect` |
| `dont_touch` | `set_dont_touch` | flag on `dbInst` and `dbNet` |

In each row the text is read once and the database carries the result.
UPF is the clearest example: `read_upf` at the start, `dbPowerDomain` and
friends thereafter, and `write_upf` to get text back out when a human
wants it.

## What is being added now: timing constraints

The user's `.sdc` is input, exactly like UPF. A flow reads it once with
`read_sdc`. From then on the constraints are design state and belong in
the `.odb`.

PR #11260 does this. `write_db -sdc` stores the constraints of every mode
in the block. `read_db` restores them when the design is linked against
a Liberty, and stores nothing and restores nothing otherwise. The `.sdc`
files that today are copied from stage to stage beside every `.odb`, and
matched to it by a directory-sorting convention in the flow, are no
longer needed between stages.

`read_sdc` and `write_sdc` are untouched. They remain the human
interface: forgiving on the way in, canonical on the way out.

## What stays out

The PDK and the analysis environment: Liberty, extraction rules, and
parasitics or constraints produced outside OpenROAD. These are what the
design was built against, not what the design owns. They are constant
across a flow, shared by many designs, and large.

They stay out for two reasons, either sufficient:

- **The `.odb` must be a function of the design alone.** Build systems
  that cache on content hashes depend on two builds of the same design
  producing the same bytes. A path, a timestamp, a hostname or a tool
  version inside the `.odb` breaks that.
- **Constant, large data does not belong in variable, numerous
  checkpoints.** A flow writes many `.odb` files per run. Copying the
  PDK into each is the wrong side of the multiplication.

Every flow already passes these files explicitly, and that does not
change.

## What is welcome

A single-concern PR that moves one piece of design state into the
`.odb`, under the rules below. Candidates known today:

| state | today | note |
|---|---|---|
| PDN grid specification | side script | the grid definition, not the shapes, which are already stored |
| analysis mode and corner names | side script | the names and their constraints; the Liberty they bind to stays out |
| parasitics extracted by rcx | side file | derived from the design, so design state; ownership to be settled |

These are invitations, not commitments. Something not listed is equally
welcome if it passes the two tests: the design owns it, and a later
stage needs it.

## Rules for a single-concern PR

Reviewers should hold PRs to these.

1. **One concern per PR.**
2. **Opt-in on the producer, automatic on the consumer.** The writer
   chooses to store. `read_db` restores what the block carries when it
   can use it, with no flag. A reader that cannot use it leaves it
   untouched.
3. **No changes to existing results.** A flow that does not opt in
   produces the same files and runs the same way.
4. **Older OpenROAD reads the file unchanged.** New state is additive.
5. **No new dependency from odb to a tool.** If a tool owns the model,
   store an opaque, versioned payload on the block and keep the encoder
   and decoder in that tool. If odb is the natural owner, add typed
   objects. Either is acceptable; a payload can become typed objects
   later.
6. **Guard against stale state.** State that references design objects
   must detect that the design changed underneath it and refuse rather
   than apply onto the wrong objects.
7. **Round trip is the test.** The tool's own writer produces
   byte-identical output before store and after restore, and store,
   restore, store is a fixpoint on the `.odb`. Log diffs are not enough.
8. **Never silently drop.** If the stored form cannot represent
   something, fall back to a complete form or refuse with an error that
   names the construct.
9. **Reproducible bytes.** No paths, timestamps, hostnames, tool
   versions, or containers serialised in iteration order.
10. **Measure.** Load time, peak memory, `.odb` size and write time,
    before and after, on at least one large design.

## Non-goals

- Making the `.odb` an archive with a user-visible loading sequence.
- Carrying PDK content, or paths to it, in the `.odb`.
- Storing logs, reports or metrics. Those are outputs, not state.
- Replacing the Tcl interface. Scripts drive the flow; the database
  carries the design between stages.

## Appendix: a sidecar, possibly

A bare `.odb` cannot be opened in the GUI with timing, because the
knowledge of which Liberty it was built against lives in the flow, not
beside the file. One concept that could address this without touching
the `.odb` is a small sidecar file with the same stem, listing the
environment files by relative path and content digest. There is ample
precedent (ELF `.gnu_debuglink`, source maps, OpenAccess `lib.defs`).

This is noted as possibly relevant, not proposed. The project has only
second-hand accounts of who would want it and what else it should hold.
Insights on the concept, and concrete use cases stating what writes the
file, what reads it, and what breaks today without it, are welcome.
