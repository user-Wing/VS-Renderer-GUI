"""Exercise the real shell adapter and isolated decoder, without registry changes."""
from pathlib import Path
import subprocess
import sys
import tempfile
import os

probe, library, ffmpeg = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
with tempfile.TemporaryDirectory(prefix="vsr-shell-") as temporary:
    root = Path(temporary)
    cases = {
        "中文 alpha.jxl": ["-frames:v", "1", "-c:v", "libjxl", "-effort", "1"],
        "av1-444-12.mkv": ["-frames:v", "4", "-c:v", "libaom-av1", "-cpu-used", "8", "-threads", "2", "-pix_fmt", "yuv444p12le"],
        "av1-420-10.mkv": ["-frames:v", "4", "-c:v", "libaom-av1", "-cpu-used", "8", "-threads", "2", "-pix_fmt", "yuv420p10le"],
        "still.avif": ["-frames:v", "1", "-c:v", "libaom-av1", "-cpu-used", "8", "-threads", "2", "-pix_fmt", "yuv444p12le"],
        "image.png": ["-frames:v", "1"],
        "lossless.webp": ["-frames:v", "1", "-c:v", "libwebp", "-lossless", "1"],
        "image.qoi": ["-frames:v", "1", "-c:v", "qoi"],
        "image.jp2": ["-frames:v", "1", "-c:v", "jpeg2000"],
        "image.exr": ["-frames:v", "1", "-c:v", "exr"],
        "rgba16.jxl": ["-frames:v", "1", "-vf", "format=rgba64le", "-c:v", "libjxl", "-distance", "0", "-effort", "1"],
    }
    for name, options in cases.items():
        path = root / name
        subprocess.run([ffmpeg, "-v", "error", "-f", "lavfi", "-i", "testsrc2=size=160x90:rate=24", *options, "-y", str(path)], check=True, timeout=30)
        modes = ["file", "stream", "memory", "persist"]
        if os.environ.get("VSR_TEST_REGISTERED_SHELL"):
            modes += ["legacy", "factory"]
        for mode in modes:
            env = dict(os.environ)
            env.pop("VSR_THUMB_STREAM", None)
            env.pop("VSR_THUMB_MEMORY", None)
            env.pop("VSR_THUMB_LEGACY", None)
            if mode in ("stream", "memory"):
                env["VSR_THUMB_STREAM"] = "1"
            if mode == "persist":
                env["VSR_THUMB_LEGACY"] = "1"
            if mode == "memory":
                env["VSR_THUMB_MEMORY"] = "1"
            result = subprocess.run([probe, str(path), mode if mode in ("legacy", "factory") else library], capture_output=True, text=True, timeout=25, env=env)
            print(name, mode, result.stdout.strip(), flush=True)
            assert result.returncode == 0 and "size=160x90" in result.stdout, result.stderr
    invalid = root / "broken.jxl"
    invalid.write_bytes(b"not an image")
    result = subprocess.run([probe, str(invalid), library], capture_output=True, text=True, timeout=25)
    assert result.returncode != 0, "Invalid media must not produce a successful thumbnail"
    print("Malformed input rejected; decoding fixtures passed.", flush=True)
