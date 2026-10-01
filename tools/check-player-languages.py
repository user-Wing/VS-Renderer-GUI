"""Keep the portable player language packs complete when UI strings change."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
packs = {name: json.loads((root / f"assets/languages/{name}.json").read_text(encoding="utf-8"))
         for name in ("zh_CN", "en_US")}
sources = list((root / "src/player").glob("*.cpp")) + [root / "src/backend/LavPlayback.cpp", root / "src/update/PortableUpdater.cpp"]
required = set()
for path in sources:
    if path.stem in ("PlayerLanguage", "PlayerAssociations", "PlayerCache"):
        continue
    for match in re.finditer(r'"((?:\\.|[^"\\])*)"', path.read_text(encoding="utf-8")):
        if re.search(r"[\u4e00-\u9fff]", match.group(1)):
            required.add(json.loads('"' + match.group(1) + '"'))
for name, entries in packs.items():
    missing = required - entries.keys()
    assert not missing, (name, sorted(missing))
for source in required:
    assert sorted(re.findall(r"%\d+", source)) == sorted(re.findall(r"%\d+", packs["en_US"][source])), source
    assert not re.search(r"[\u4e00-\u9fff]", packs["en_US"][source]), source
print(f"PASS: {len(required)} player strings, both language packs, matching placeholders")
