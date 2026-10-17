from bisect import bisect_right as _vsr_bd_bisect
from threading import Lock as _vsr_bd_lock


def _vsr_bd_source(loader, args, options, lsmas):
    global _vsr_bd_reference_frame
    source = args[0] if args else options.get('source')
    if source != _vsr_bd_input:
        return loader(*args, **options)
    options = dict(options)
    options.pop('source', None)
    options.pop('cachedir', None)
    seconds = [0.0]
    for row in _vsr_bd_parts:
        seconds.append(seconds[-1] + row[2])
    current = min(len(_vsr_bd_parts) - 1, _vsr_bd_bisect(seconds, _vsr_bd_resume) - 1)
    sources = {}

    def segment(index):
        path, start, duration, ffindex, lwi = _vsr_bd_parts[index]
        if path not in sources:
            opts = dict(options)
            cache = lwi if lsmas else ffindex
            opts.update(cache=int(bool(cache)), cachefile=cache)
            opts.setdefault('threads', _vsr_bd_threads)
            sources[path] = loader(path, *args[1:], **opts)
        clip = sources[path]
        first = max(0, round(start * clip.fps_num / clip.fps_den))
        count = round(duration * clip.fps_num / clip.fps_den)
        return clip[first:first + count]

    reference = segment(current)
    _vsr_bd_reference_frame = reference.get_frame(0)
    counts = [round(row[2] * reference.fps_num / reference.fps_den) for row in _vsr_bd_parts]
    offsets = [0]
    for count in counts:
        offsets.append(offsets[-1] + count)
    blank = _vsr_vs.core.std.BlankClip(reference, length=offsets[-1])
    clips = {}

    def timeline(index, clip):
        parts = []
        if offsets[index]:
            parts.append(blank[:offsets[index]])
        parts.append(clip)
        if offsets[index + 1] < offsets[-1]:
            parts.append(blank[offsets[index + 1]:])
        return _vsr_vs.core.std.Splice(parts)

    clips[current] = timeline(current, reference)

    lock = _vsr_bd_lock()

    def select(n):
        index = _vsr_bd_bisect(offsets, n) - 1
        with lock:
            if index not in clips:
                clips[index] = timeline(index, segment(index))
            return clips[index]

    return _vsr_vs.core.std.FrameEval(blank, eval=select)


def _vsr_bd_ffms(*args, **options):
    return _vsr_bd_source(_vsr_vs.core.ffms2.Source, args, options, False)


def _vsr_bd_lsmas(*args, **options):
    return _vsr_bd_source(_vsr_vs.core.lsmas.LWLibavSource, args, options, True)
