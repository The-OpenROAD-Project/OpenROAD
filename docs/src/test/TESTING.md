# Documentation Script Tests

Unit tests for the Python scripts that convert module `README.md` files
into man-page markdown (`md_roff_compat.py`, `extract_utils.py`).

## What is tested

| Test file | Covers |
| --------- | ------ |
| `test_extract_utils.py` | `extract_headers`, `extract_tcl_command`, `extract_tcl_code`, `extract_description`, `extract_arguments`, `extract_tables`, `parse_switch`, `check_function_signatures` |
| `test_md_roff_compat.py` | `man2_translate` (valid input, count mismatch, error message content), `man3_translate` (valid input, `with-total` guard, per-line write calls) |

`man2_translate` and `man3_translate` use `unittest.mock` to replace
`ManPage.write_roff_file` where needed, so no files outside `tmp_path`
are written. The full suite runs in under 0.1 s.

## Running locally

```bash
# from the docs/ directory
pip install pytest pytest-cov   # once; already in requirements.txt
make test
```

Or directly:

```bash
cd docs/src/test
python3 -m pytest test_extract_utils.py test_md_roff_compat.py -v \
    --cov=../scripts --cov-report=term-missing
```

## CI integration

The `preprocess` target (called by Jenkins as `make preprocess -C docs`)
runs the integration-level check: it links all module READMEs and runs
`md_roff_compat.py` over every one of them, raising `ValueError` on any
structural mismatch.

The unit tests are run via `make test` and should be added as a separate
CI step alongside `make preprocess`. To add it to a GitHub Actions
workflow:

```yaml
- name: Run doc script unit tests
  run: |
    pip install -r docs/requirements.txt
    make -C docs test
```
