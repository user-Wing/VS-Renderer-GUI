"""Normalize a clean portable staging tree before computing its release manifest."""
from pathlib import Path
import argparse
import shutil

parser = argparse.ArgumentParser()
parser.add_argument("stage", type=Path)
parser.add_argument("--single", action="store_true")
args = parser.parse_args()
stage = args.stage.resolve()
project = Path(__file__).resolve().parents[1]
if not stage.is_relative_to(project / "build") or not (stage / "vs-player.exe").is_file():
    raise SystemExit("Expected a player staging directory under build")


def move(source, destination):
    if not source.exists():
        return
    if source.resolve() == destination.resolve():
        if source.name != destination.name and source.parent == destination.parent:
            source.rename(source.with_name(source.name+'.renaming')).rename(destination)
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    if source.is_dir() and destination.is_dir():
        for child in source.iterdir():
            move(child, destination / child.name)
        source.rmdir()
    elif destination.exists():
        if source.read_bytes() != destination.read_bytes():
            raise ValueError(f"Conflicting staged files: {source}, {destination}")
        source.unlink()
    else:
        source.rename(destination)


tools = stage / "tools"
for folder in ("styles", "iconengines", "imageformats", "tls", "platforms", "networkinformation"):
    move(stage / folder, stage)
runtime = stage / "runtime"
for item in runtime.glob("*"):
    if item.name == "ncnn":
        shutil.rmtree(item)
    elif item.name == "tools":
        for file in item.iterdir():
            if file.name in ("7z.exe", "7z.dll"):
                target = tools / "7z" / file.name
            elif file.name == "aria2-next.exe":
                target = tools / "aria2-next" / file.name
            elif file.suffix.lower() in (".txt", ".md"):
                target = stage / "licenses" / ("aria2-next-License.txt" if "aria" in file.name else "7-Zip-License.txt")
            else:
                target = tools / "update" / file.name
            move(file, target)
        item.rmdir()
    elif item.name == "awj-source":
        move(item, stage / "licenses" / "AWJ")
    else:
        folder = {"python":"vapoursynth","awj":"awj-s"}.get(item.stem, item.stem)
        move(item, tools / folder if item.is_dir() else tools / folder / item.name)
if runtime.exists():
    runtime.rmdir()
for old,new in (("community-markdown","tools/vditor"),("plugins/3fui-export-bridge","tools/3fui-export"),
                ("themes","config/skin"),("luts","config/LUT"),("languages","config/languages"),
                ("filter-licenses","licenses/filters"),("rife-licenses","licenses/RIFE"),("shader-licenses","licenses/shaders"),
                ("assets","docs/assets")):
    move(stage / old, stage / new)
for old, new in (("tools/download-bd.py", "tools/bd-download/download-bd.py"),
                 ("plugins/3FUI/VsrExportBridge.3fui.dll", "tools/3fui-export/VsrExportBridge.3fui.dll"),
                 ("plugins/3FUI/README.md", "docs/3fui-install.md"),
                 ("WebView2-LICENSE.txt", "licenses/WebView2-License.txt"),
                 ("player.ini", "config/player.ini"), ("vs-cap.ini", "config/cap.ini"),
                 ("themes.ini", "config/themes.ini"), ("shaders/custom", "config/shaders/custom")):
    move(stage / old, stage / new)
for directory in (stage / "plugins/3FUI", stage / "plugins"):
    if directory.is_dir() and not any(directory.iterdir()):
        directory.rmdir()
readme=stage / "README.md"
if readme.is_file() and b"(assets/agent-preview.png)" in readme.read_bytes():
    preview=stage / "docs/assets/agent-preview.png"
    preview.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(project / "assets/agent-preview.png",preview)
    readme.write_bytes(readme.read_bytes().replace(b"(assets/agent-preview.png)",b"(docs/assets/agent-preview.png)"))
guide = stage / "docs/assets/ai-subtitles/README.md"
guide.parent.mkdir(parents=True, exist_ok=True)
shutil.copyfile(project / "assets/ai-subtitles/README.md", guide)
for document, before, after in (
    ("docs/player.md", b"(../assets/ai-subtitles/README.md)", b"(assets/ai-subtitles/README.md)"),
    ("project.md", b"(assets/ai-subtitles/README.md)", b"(docs/assets/ai-subtitles/README.md)"),
):
    path = stage / document
    if path.is_file():
        path.write_bytes(path.read_bytes().replace(before, after))
licenses = stage / "licenses"
for file in list(licenses.rglob("*")):
    if file.is_file() and file.suffix.lower() in (".glsl",".hlsl"):
        file.unlink()
    elif file.is_file() and file.parent == licenses and file.suffix.lower() in (".txt",".md"):
        name=file.stem
        for suffix in ("-license","-copying"):
            if name.lower().endswith(suffix):
                name=name[:-len(suffix)]
                break
        if "notice" not in name.lower():
            move(file,licenses / (name + "-License" + file.suffix.lower()))
if args.single:
    # PlayerLanguage seeds editable defaults from its Qt resources on first launch.
    shutil.rmtree(stage / "config",ignore_errors=True)
else:
    for preset in (stage / "vpy").glob("*.vpy"):
        move(preset,stage / "vpy/user" / preset.name)
    for name in ("skin","languages","LUT"):
        (stage / "config" / name).mkdir(parents=True,exist_ok=True)
    for name in ("builtin","user","community"):
        (stage / "vpy" / name).mkdir(parents=True,exist_ok=True)
