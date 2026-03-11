## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

import pytest
from extract_utils import (
    check_function_signatures,
    extract_arguments,
    extract_description,
    extract_headers,
    extract_tables,
    extract_tcl_code,
    extract_tcl_command,
    parse_switch,
)

# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

MINIMAL_MD = """\
## Commands

### Foo Command

Foo description.

```tcl
foo_cmd
    [-opt val]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-opt` | An option. |

## License

BSD 3-Clause.
"""

TWO_SECTION_MD = """\
## Commands

### First Command

First description.

```tcl
first_command
    [-a val]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-a` | Option a. |

#### Extra Args

| Switch Name | Description |
| ---- | ---- |
| `pos` | Positional arg. |

### Second Command

Second description.

```tcl
second_command
    [-b val]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-b` | Option b. |

## License

BSD 3-Clause.
"""


# ---------------------------------------------------------------------------
# extract_headers
# ---------------------------------------------------------------------------


class TestExtractHeaders:
    def test_level1(self):
        assert extract_headers("# Hello\n## World\n", 1) == ["Hello"]

    def test_level2(self):
        assert extract_headers("# Hello\n## World\n", 2) == ["World"]

    def test_level3_multiple(self):
        assert extract_headers("### Foo\n### Bar\n", 3) == ["Foo", "Bar"]

    def test_empty_text(self):
        assert extract_headers("no headers", 2) == []

    def test_invalid_level_zero(self):
        with pytest.raises(ValueError):
            extract_headers("# Hi", 0)

    def test_invalid_level_negative(self):
        with pytest.raises(ValueError):
            extract_headers("# Hi", -1)

    def test_invalid_level_string(self):
        with pytest.raises(ValueError):
            extract_headers("# Hi", "1")


# ---------------------------------------------------------------------------
# extract_tcl_command
# ---------------------------------------------------------------------------


class TestExtractTclCommand:
    def test_single(self):
        md = "```tcl\nfoo_bar\n    [-opt]\n```\n"
        assert extract_tcl_command(md) == ["foo_bar"]

    def test_multiple(self):
        md = "```tcl\ncmd_one\n```\n```tcl\ncmd_two\n```\n"
        assert extract_tcl_command(md) == ["cmd_one", "cmd_two"]

    def test_no_tcl_blocks(self):
        assert extract_tcl_command("No TCL here.") == []

    def test_namespace_command(self):
        md = "```tcl\ngui::show\n```\n"
        assert extract_tcl_command(md) == ["gui::show"]


# ---------------------------------------------------------------------------
# extract_tcl_code
# ---------------------------------------------------------------------------


class TestExtractTclCode:
    def test_single_block(self):
        md = "```tcl\nfoo_bar\n    [-opt]\n```\n"
        result = extract_tcl_code(md)
        assert len(result) == 1
        assert "foo_bar" in result[0]

    def test_filters_gcd_script(self):
        md = "```tcl\n./test/gcd.tcl\n```\n```tcl\nreal_cmd\n```\n"
        result = extract_tcl_code(md)
        assert len(result) == 1
        assert "real_cmd" in result[0]

    def test_multiple_blocks(self):
        md = "```tcl\ncmd1\n```\n```tcl\ncmd2\n```\n"
        assert len(extract_tcl_code(md)) == 2

    def test_no_blocks(self):
        assert extract_tcl_code("plain text") == []


# ---------------------------------------------------------------------------
# extract_description
# ---------------------------------------------------------------------------


class TestExtractDescription:
    def test_single_section(self):
        descs = extract_description(MINIMAL_MD)
        assert len(descs) == 1
        assert "Foo description" in descs[0]

    def test_two_sections(self):
        descs = extract_description(TWO_SECTION_MD)
        assert len(descs) == 2
        assert "First description" in descs[0]
        assert "Second description" in descs[1]

    def test_strips_whitespace(self):
        descs = extract_description(MINIMAL_MD)
        assert descs[0] == descs[0].strip()


# ---------------------------------------------------------------------------
# extract_arguments
# ---------------------------------------------------------------------------


class TestExtractArguments:
    def test_minimal_has_options(self):
        options, _ = extract_arguments(MINIMAL_MD)
        assert len(options) == 1
        assert any("-opt" in row for row in options[0])

    def test_minimal_no_args(self):
        _, args = extract_arguments(MINIMAL_MD)
        assert args[0] == []

    def test_two_sections_counts_match(self):
        options, args = extract_arguments(TWO_SECTION_MD)
        assert len(options) == 2
        assert len(args) == 2

    def test_extra_args_table_captured(self):
        _, args = extract_arguments(TWO_SECTION_MD)
        assert any("pos" in row for row in args[0])

    def test_section_without_options(self):
        md = (
            "## Commands\n\n"
            "### No Options Command\n\n"
            "Just a description.\n\n"
            "```tcl\nno_opts_cmd\n```\n\n"
            "## License\n"
        )
        options, args = extract_arguments(md)
        assert options[0] == []
        assert args[0] == []

    def test_no_level3_returns_empty(self):
        md = "## Commands\n\nNo sections.\n\n## License\n"
        options, args = extract_arguments(md)
        assert options == []
        assert args == []

    def test_no_level2_after_last_level3_raises(self):
        md = "## Commands\n\n### Only Section\n\nDesc.\n\n```tcl\ncmd\n```\n"
        with pytest.raises(ValueError, match="No level-2 header found"):
            extract_arguments(md)


# ---------------------------------------------------------------------------
# extract_tables
# ---------------------------------------------------------------------------


class TestExtractTables:
    def test_basic_row(self):
        text = "#### Options\n| `-opt` | An option. |\n"
        assert any("-opt" in r for r in extract_tables(text))

    def test_skips_header_row(self):
        text = "| Switch Name | Description |\n| ---- | ---- |\n| `-opt` | desc |\n"
        rows = extract_tables(text)
        assert not any("Switch Name" in r for r in rows)
        assert not any("---" in r for r in rows)

    def test_skips_html(self):
        text = "| `<b>opt</b>` | desc |\n| `-real` | real |\n"
        rows = extract_tables(text)
        assert not any("<b>" in r for r in rows)
        assert any("-real" in r for r in rows)

    def test_empty_text(self):
        assert extract_tables("no table here") == []


# ---------------------------------------------------------------------------
# parse_switch
# ---------------------------------------------------------------------------


class TestParseSwitch:
    def test_simple(self):
        key, val = parse_switch("| `-opt` | An option. |")
        assert key == "-opt"
        assert "An option" in val

    def test_backticks_stripped(self):
        key, _ = parse_switch("| `name` | The name. |")
        assert key == "name"

    def test_pipe_in_description(self):
        # Content containing | — key must still be correct
        key, _ = parse_switch("| `-flag` | Either a | b. |")
        assert key == "-flag"


# ---------------------------------------------------------------------------
# check_function_signatures
# ---------------------------------------------------------------------------


class TestCheckFunctionSignatures:
    def test_matching(self):
        assert check_function_signatures("-a -b -c", "-c -b -a") is True

    def test_mismatch_returns_false(self, capsys):
        assert check_function_signatures("-a -b", "-a -c") is False

    def test_mismatch_prints_diff(self, capsys):
        check_function_signatures("-a -b", "-a -c")
        out = capsys.readouterr().out
        assert "-b" in out or "-c" in out
