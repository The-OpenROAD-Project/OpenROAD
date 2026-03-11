## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

import os
from unittest.mock import MagicMock, patch

import pytest

from md_roff_compat import man2_translate, man3_translate

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

# Two level-3 headers but only one ```tcl block — triggers the count check.
MISMATCH_MD = """\
## Commands

### Real Command

Real description.

```tcl
real_cmd
```

### Feature Description

No TCL block here.

## License

BSD 3-Clause.
"""

# Three message lines in the format used by messages.txt files.
MESSAGES_TXT = (
    "GPL 0002 file.cpp:778 DBU: {} INFO https://example.com\n"
    "GPL 0003 file.cpp:806 SiteSize: {} {} INFO https://example.com\n"
    "GPL 0004 file.cpp:807 CoreAreaLxLy: {} {} INFO https://example.com\n"
)


# ---------------------------------------------------------------------------
# man2_translate
# ---------------------------------------------------------------------------


class TestMan2Translate:
    def test_valid_md_writes_manpage(self, tmp_path):
        doc = tmp_path / "test.md"
        doc.write_text(MINIMAL_MD, encoding="utf-8")
        man2_translate(str(doc), str(tmp_path))
        assert (tmp_path / "foo_cmd.md").exists()

    def test_count_mismatch_raises_value_error(self, tmp_path):
        doc = tmp_path / "mismatch.md"
        doc.write_text(MISMATCH_MD, encoding="utf-8")
        with pytest.raises(ValueError, match="Documentation parse error"):
            man2_translate(str(doc), str(tmp_path))

    def test_error_message_names_the_file(self, tmp_path):
        doc = tmp_path / "mymodule.md"
        doc.write_text(MISMATCH_MD, encoding="utf-8")
        with pytest.raises(ValueError, match="mymodule.md"):
            man2_translate(str(doc), str(tmp_path))

    def test_error_message_shows_common_causes(self, tmp_path):
        doc = tmp_path / "mismatch.md"
        doc.write_text(MISMATCH_MD, encoding="utf-8")
        with pytest.raises(ValueError, match="###"):
            man2_translate(str(doc), str(tmp_path))

    def test_manpage_write_called_for_each_function(self, tmp_path):
        doc = tmp_path / "test.md"
        doc.write_text(MINIMAL_MD, encoding="utf-8")
        with patch("md_roff_compat.ManPage") as MockManPage:
            instance = MagicMock()
            MockManPage.return_value = instance
            man2_translate(str(doc), str(tmp_path))
        instance.write_roff_file.assert_called_once_with(str(tmp_path))

    def test_translator_sample_parses_cleanly(self, tmp_path):
        """Smoke-test: the project's own sample fixture must parse without error."""
        sample = os.path.join(os.path.dirname(__file__), "translator.md")
        man2_translate(sample, str(tmp_path))


# ---------------------------------------------------------------------------
# man3_translate
# ---------------------------------------------------------------------------


class TestMan3Translate:
    def test_valid_messages_produce_manpages(self, tmp_path):
        doc = tmp_path / "messages.txt"
        doc.write_text(MESSAGES_TXT, encoding="utf-8")
        man3_translate(str(doc), str(tmp_path))
        assert (tmp_path / "GPL-0002.md").exists()
        assert (tmp_path / "GPL-0003.md").exists()
        assert (tmp_path / "GPL-0004.md").exists()

    def test_with_total_in_name_raises(self, tmp_path):
        # "with-total" appearing as the num field ends up in manpage.name.
        doc = tmp_path / "bad.txt"
        doc.write_text(
            "GPL with-total file.cpp:1 message INFO https://example.com\n",
            encoding="utf-8",
        )
        with pytest.raises(ValueError, match="with-total"):
            man3_translate(str(doc), str(tmp_path))

    def test_error_names_the_file(self, tmp_path):
        doc = tmp_path / "badmsgs.txt"
        doc.write_text(
            "GPL with-total file.cpp:1 message INFO https://example.com\n",
            encoding="utf-8",
        )
        with pytest.raises(ValueError, match="badmsgs.txt"):
            man3_translate(str(doc), str(tmp_path))

    def test_manpage_write_called_per_line(self, tmp_path):
        doc = tmp_path / "messages.txt"
        doc.write_text(MESSAGES_TXT, encoding="utf-8")
        with patch("md_roff_compat.ManPage") as MockManPage:
            instance = MagicMock()
            MockManPage.return_value = instance
            man3_translate(str(doc), str(tmp_path))
        assert instance.write_roff_file.call_count == 3
