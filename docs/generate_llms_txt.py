import re
from pathlib import Path
import yaml

DOCS_DIR = Path(__file__).parent
REPO_ROOT = DOCS_DIR.parent
BASE_URL = "https://openroad.readthedocs.io/en/latest/"

# Excluded from llms-full.txt: auto-generated and too large/noisy for LLMs
FULL_TXT_EXCLUDE = {"user/MessagesFinal", "user/MessagesFinal.md"}

# Agent guides live in docs/agents/ but are not in toc.yml
EXTRA_AGENT_DOCS = [
    ("Build Guide (AI Agents)", "agents/build.md"),
    ("Testing Guide (AI Agents)", "agents/testing.md"),
    ("CI Guide (AI Agents)", "agents/ci.md"),
    ("Coding Patterns (AI Agents)", "agents/coding.md"),
]

PROJECT_SUMMARY = """\
> OpenROAD is an open-source, fully-autonomous RTL-to-GDSII tool chain for digital SoC layout
> generation, targeting no-human-in-loop design with 24-hour turnaround. It lowers barriers of
> cost, expertise, and unpredictability in hardware implementation. Maintained by UC San Diego
> and partners (Qualcomm, Arm, and multiple universities) under the DARPA IDEA program.
> Docs: https://openroad.readthedocs.io | GitHub: https://github.com/The-OpenROAD-Project/OpenROAD
> Contact: openroad@ucsd.edu"""


def _try_path(base: Path, rel: str) -> Path | None:
    """Try base/rel and base/rel.md, return first that exists."""
    p = base / rel
    if p.exists():
        return p
    if not rel.endswith(".md"):
        p2 = base / (rel + ".md")
        if p2.exists():
            return p2
    return None


def resolve_file(toc_path: str) -> Path | None:
    """Resolve a toc.yml file reference to an actual .md path.

    Handles two cases:
    - paths starting with "main/" use a symlink docs/main -> .. (Sphinx build time);
      fall back to REPO_ROOT when the symlink does not exist (standalone runs).
    - all other paths are relative to DOCS_DIR.
    """
    result = _try_path(DOCS_DIR, toc_path)
    if result:
        return result
    if toc_path.startswith("main/"):
        return _try_path(REPO_ROOT, toc_path[len("main/"):])
    return None


def extract_description(md_path: Path, max_len: int = 120) -> str:
    """Return first substantive prose paragraph after the H1, stripped of markdown."""
    try:
        text = md_path.read_text(errors="replace")
    except Exception:
        return ""

    lines = text.splitlines()
    in_code = False
    found_h1 = False
    para: list[str] = []

    for line in lines:
        s = line.strip()

        if s.startswith("```") or s.startswith(":::"):
            in_code = not in_code
            continue
        if in_code:
            continue

        if not found_h1:
            if s.startswith("# "):
                found_h1 = True
            continue

        if s.startswith("#"):
            if para:
                break
            continue

        # Skip table rows, images/badges, HTML, horizontal rules
        if (s.startswith("|") or s.startswith("![") or s.startswith("<")
                or re.match(r"\[!\[", s) or s.startswith("---")):
            if para:
                break
            continue

        if not s:
            if para:
                break
            continue

        # Skip leading list items — prefer prose paragraphs
        if re.match(r"^[-*]|\d+\.", s) and not para:
            continue

        para.append(s)

    if not para:
        return ""

    desc = " ".join(para)
    # Strip inline markdown syntax
    desc = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", desc)
    desc = re.sub(r"[`*_]", "", desc)
    desc = re.sub(r"\s+", " ", desc).strip()

    if len(desc) > max_len:
        desc = desc[:max_len].rsplit(" ", 1)[0] + "..."
    return desc


def make_url(toc_path: str) -> str:
    """Map a toc.yml file path to its ReadTheDocs HTML URL."""
    return BASE_URL + toc_path.removesuffix(".md") + ".html"


def flatten_toc(entries: list, depth: int = 0) -> list[tuple[int, str, str, bool]]:
    """Recursively flatten toc.yml entries.

    Returns list of (depth, title, path_or_url, is_external_url).
    """
    result = []
    for entry in entries:
        title = entry.get("title", "")
        if "url" in entry:
            result.append((depth, title, entry["url"], True))
        elif "file" in entry:
            result.append((depth, title, entry["file"], False))
        if "entries" in entry:
            result.extend(flatten_toc(entry["entries"], depth + 1))
    return result


def load_toc() -> list[tuple[int, str, str, bool]]:
    with open(DOCS_DIR / "toc.yml") as f:
        toc = yaml.safe_load(f)
    return flatten_toc(toc.get("entries", []))


def _link_line(title: str, url: str, md_path: Path | None) -> str:
    desc = extract_description(md_path) if md_path else ""
    line = f"- [{title}]({url})"
    if desc:
        line += f": {desc}"
    return line


def generate_llms_txt(entries: list, output_path: Path) -> None:
    lines = ["# OpenROAD", "", PROJECT_SUMMARY, ""]

    for depth, title, path, is_external in entries:
        if depth == 0:
            lines.append(f"## {title}")
            lines.append("")
            if not is_external:
                lines.append(_link_line(title, make_url(path), resolve_file(path)))
        else:
            if is_external:
                lines.append(f"- [{title}]({path}): External documentation")
            else:
                lines.append(_link_line(title, make_url(path), resolve_file(path)))

    lines.append("")
    output_path.write_text("\n".join(lines))
    print(f"Generated {output_path} ({output_path.stat().st_size:,} bytes)")


def generate_llms_full_txt(entries: list, output_path: Path) -> None:
    parts = ["# OpenROAD", "", PROJECT_SUMMARY]
    seen: set[str] = set()

    for _, title, path, is_external in entries:
        if is_external:
            continue
        norm = path.removesuffix(".md")
        if norm in FULL_TXT_EXCLUDE or path in FULL_TXT_EXCLUDE:
            continue
        if norm in seen:
            continue
        seen.add(norm)

        md = resolve_file(path)
        if not md:
            continue
        try:
            content = md.read_text(errors="replace").strip()
        except Exception:
            continue

        parts += ["", "---", "", f"# {title}", "", content]

    # Append agent guides not present in toc.yml
    for title, rel_path in EXTRA_AGENT_DOCS:
        agent_file = DOCS_DIR / rel_path
        if agent_file.exists():
            parts += [
                "",
                "---",
                "",
                f"# {title}",
                "",
                agent_file.read_text(errors="replace").strip(),
            ]

    parts.append("")
    output_path.write_text("\n".join(parts))
    print(f"Generated {output_path} ({output_path.stat().st_size:,} bytes)")


if __name__ == "__main__":
    entries = load_toc()
    generate_llms_txt(entries, DOCS_DIR / "llms.txt")
    generate_llms_full_txt(entries, DOCS_DIR / "llms-full.txt")
