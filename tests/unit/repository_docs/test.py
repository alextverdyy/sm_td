import re
import unittest
from pathlib import Path
from urllib.parse import unquote, urlparse


ROOT = Path(__file__).resolve().parents[3]
MARKDOWN_LINK = re.compile(r"\[[^]]+\]\(([^)]+)\)")
DOC_FILES = [ROOT / "README.md", ROOT / "CONTRIBUTING.md", *sorted((ROOT / "docs").glob("*.md"))]


class TestRepositoryDocumentation(unittest.TestCase):
    def test_relative_markdown_links_resolve(self):
        broken = []
        for document in DOC_FILES:
            for raw_target in MARKDOWN_LINK.findall(document.read_text(errors="replace")):
                target = raw_target.strip().split(maxsplit=1)[0].strip("<>")
                parsed = urlparse(target)
                if parsed.scheme or target.startswith("#"):
                    continue
                path_text = unquote(parsed.path)
                if not path_text:
                    continue
                destination = (ROOT / path_text.lstrip("/")) if path_text.startswith("/") else (document.parent / path_text)
                if not destination.resolve().exists():
                    broken.append(f"{document.relative_to(ROOT)} -> {target}")

        self.assertEqual(broken, [], "broken relative documentation links:\n" + "\n".join(broken))

    def test_documentation_index_covers_numbered_guides(self):
        index = (ROOT / "docs" / "README.md").read_text()
        missing = [guide.name for guide in sorted((ROOT / "docs").glob("[0-9][0-9][0-9]_*.md")) if guide.name not in index]
        self.assertEqual(missing, [], "guides missing from docs/README.md")


if __name__ == "__main__":
    unittest.main()
