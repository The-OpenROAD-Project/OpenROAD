---
name: triage-issue
description: >
  Triage an OpenROAD GitHub issue by reproducing the bug and
  minimizing the test case with whittle.py. Use when an issue
  has an attached tarball artifact (.tar.gz) from `make *_issue`.
argument-hint: <issue-number>
---

# Triage OpenROAD Issue

You are triaging OpenROAD issue **$ARGUMENTS**.

## 1. Fetch issue details

```bash
gh issue view $ARGUMENTS --repo The-OpenROAD-Project/OpenROAD \
  --json title,body,labels,state,comments
```

Look for:
- Error string (e.g. `GPL-0305`, `GPL-0307`, `DRT-XXXX`)
- Attached tarball URL (from `make *_issue`)
- OpenROAD version the bug was reported against

## 2. Download and extract artifact

Download any `.tar.gz` attached to the issue body or comments.
Extract into a working directory:

```bash
mkdir -p /tmp/triage-$ARGUMENTS
cd /tmp/triage-$ARGUMENTS
tar xzf <downloaded-tarball>
cd <extracted-dir>
```

## 3. Fix the run script

The `run-me-*.sh` scripts from `make *_issue` often need two fixes:

```bash
# Add set -e to exit on error
sed -i '2i set -e' run-me-*.sh

# Add -exit so openroad exits after the tcl script
sed -i 's/openroad -no_init/openroad -exit -no_init/g' run-me-*.sh
sed -i 's/openroad -no_splash/openroad -exit -no_splash/g' run-me-*.sh
```

## 4. Build OpenROAD

Build from the OpenROAD source tree:

```bash
cd <path-to>/OpenROAD
bazelisk build :openroad
```

The binary is at `bazel-bin/openroad`. Export it:

```bash
export OPENROAD_EXE=$(pwd)/bazel-bin/openroad
```

## 5. Reproduce the bug

Run the step command and confirm the error string appears:

```bash
cd /tmp/triage-$ARGUMENTS/<extracted-dir>
./run-me-*.sh 2>&1 | tee reproduce.log
grep -c "<error-string>" reproduce.log
```

If the bug does NOT reproduce with the latest OpenROAD, report this
on the issue -- the bug may have been fixed.

## 6. Run whittle.py

```bash
python3 <path-to>/OpenROAD/etc/whittle.py \
  --error_string "<error-string>" \
  --base_db_path <path-to>.odb \
  --use_stdout --exit_early_on_error \
  --step "./run-me-*.sh" \
  --persistence 3 --multiplier 2
```

### Monitoring progress

Watch the `[whittle]` lines. Key things to check:

- **ETA growing?** The design may be too large for the current
  persistence level. Try `--persistence 1` for a coarse first pass.
- **.odb size not shrinking?** The error may depend on most of the
  design. Try increasing `--multiplier`.
- **Step timeout too long?** Use `--timeout 300` to cap each step
  at 5 minutes.
- **After 5 minutes** of a step, whittle.py shows the tail of the
  step's log so you can see what it is doing.

### When to abort

- If after 20+ steps the .odb size has not changed, abort and try
  different parameters.
- If the step itself takes >10 minutes, consider whether the error
  string is specific enough (avoid generic strings like "ERROR").

## 7. Upload results

Create a new tarball with the whittled .odb:

```bash
cp whittle_base_result_*.odb <original-odb-path>
tar czf whittled-$ARGUMENTS.tar.gz <extracted-dir>/
```

Comment on the issue with before/after stats:

```bash
gh issue comment $ARGUMENTS --repo The-OpenROAD-Project/OpenROAD \
  --body "Whittled test case: original NNN insts -> MMM insts, \
.odb size: XXX MB -> YYY MB. Attached whittled tarball."
```

Upload the tarball to the issue comment via the GitHub web UI
(gh CLI does not support file attachments on comments).
