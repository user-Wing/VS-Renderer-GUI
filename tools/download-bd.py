"""Download a complete BD folder using an installed ModelScope Manager engine."""
import argparse
import json
import os
from pathlib import Path
import sys
import time
from dataclasses import replace
import hashlib

parser = argparse.ArgumentParser()
parser.add_argument("--manager", required=True, type=Path)
parser.add_argument("--repo", default="ARXChem/Animations-List")
parser.add_argument("--prefix", required=True)
parser.add_argument("--destination", required=True, type=Path)
parser.add_argument("--metadata-only", action="store_true")
parser.add_argument("--only-file", action="append", default=[], help="Optional file names for a targeted verification download")
parser.add_argument("--sequential", action="store_true", help="Use Manager's sequential HTTP downloader when native segmented state cannot recover")
args = parser.parse_args()
args.destination.mkdir(parents=True, exist_ok=True)
sys.stdout = sys.stderr = (args.destination / "bd-download.log").open("a", encoding="utf-8", buffering=1)
sys.path[:0] = [str(args.manager), str(args.manager / "runtime/Lib/site-packages")]
from modelscope_manager.service import ModelScopeService, Repository, RemoteEntry
from modelscope_manager.download_service import build_download_specs, Aria2DownloadRunner, Aria2Tuning

service = ModelScopeService(os.environ.get("MODELSCOPE_API_TOKEN", ""), require_token=False)
repo = Repository(args.repo, "dataset")
entries = service.list_entries(repo)
if args.metadata_only:
    entries = [e for e in entries if e.is_dir or Path(e.path).suffix.lower() in {".mpls", ".clpi", ".bdmv", ".xml"}]
if args.only_file:
    entries = [e for e in entries if Path(e.path).name in args.only_file]
specs = build_download_specs(service, repo, entries, RemoteEntry(args.prefix.strip("/"), is_dir=True), args.destination)
if not specs:
    raise SystemExit("Selected BD folder has no files")
print(json.dumps({"files": len(specs), "bytes": sum(s.size for s in specs), "destination": str(args.destination)}, ensure_ascii=False), flush=True)
runner = Aria2DownloadRunner(args.manager / "runtime/tools/aria2-next.exe", service.token,
    tuning=Aria2Tuning(medium_segments=1, large_segments=1), state_dir=args.destination / ".download-state")
last = 0.0
retry = []
def progress(done, total, speed, eta):
    global last
    if time.monotonic() - last > 10:
        print(json.dumps({"done": done, "total": total, "speed": speed, "eta": eta}), flush=True)
        last = time.monotonic()
def item(spec, state, done, total, message):
    if state in {"completed", "failed"}:
        print(json.dumps({"file": spec.remote_path, "state": state, "message": message}, ensure_ascii=False), flush=True)
    if state == "failed":
        retry.append(spec)
def sequential(selected):
    ok = failed = 0
    for spec in selected:
        temp = Path(str(spec.local_path) + ".download-part")
        print(json.dumps({"file": spec.remote_path, "state": "sequential-download"}, ensure_ascii=False), flush=True)
        try:
            temp.parent.mkdir(parents=True, exist_ok=True)
            service.download_to_file(repo, spec.remote_path, temp)
            if spec.size and temp.stat().st_size != spec.size:
                raise ValueError("Downloaded size mismatch")
            digest = hashlib.sha256()
            with temp.open("rb") as stream:
                for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b""):
                    digest.update(chunk)
            if spec.sha256 and digest.hexdigest().lower() != spec.sha256.lower():
                raise ValueError("Downloaded SHA-256 mismatch")
            temp.replace(spec.local_path)
            ok += 1
            print(json.dumps({"file": spec.remote_path, "state": "completed", "sha256": digest.hexdigest()}, ensure_ascii=False), flush=True)
        except Exception as error:
            failed += 1
            print(json.dumps({"file": spec.remote_path, "state": "failed", "message": str(error)}, ensure_ascii=False), flush=True)
    return ok, failed
ok, failed = sequential(specs) if args.sequential else runner.run(specs, progress, item)
if retry:
    # Retry only verified failures with a fresh native state database and one segment.
    failed_specs = [replace(spec, conflict_mode="overwrite") for spec in retry]
    retry.clear()
    runner = Aria2DownloadRunner(args.manager / "runtime/tools/aria2-next.exe", service.token,
        tuning=Aria2Tuning(medium_segments=1, large_segments=1), state_dir=args.destination / ".download-retry")
    retry_ok, failed = runner.run(failed_specs, progress, item)
    ok += retry_ok
    if retry:
        sequential_ok, failed = sequential(retry)
        ok += sequential_ok
print(json.dumps({"completed": ok, "failed": failed}), flush=True)
raise SystemExit(1 if failed else 0)
