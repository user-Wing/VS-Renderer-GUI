def _vsr_ncnn(c, directory, model, gpu):
    # Never spend NCNN inference time in the realtime player or on video clips.
    if globals().get('_vsr_realtime_player', False) or c.num_frames != 1:
        return c
    import ctypes
    import json
    import os
    import subprocess
    import tempfile
    import vapoursynth as vs
    if c.get_frame(0).props.get('_Transfer', 2) in (16, 18):
        raise ValueError('NCNN still-image models require SDR input')
    runtime = os.path.join(directory, 'runtime', 'ncnn')
    with open(os.path.join(runtime, 'models.json'), encoding='utf-8') as f:
        entry = next(item for item in json.load(f) if item['name'] == model)
    rgb = vs.core.resize.Bicubic(c, format=vs.RGB24, **({'matrix_in': _vsr_matrix(c)} if c.format.color_family == vs.YUV else {}))
    frame = rgb.get_frame(0)
    packed = bytearray(c.width * c.height * 3)
    for channel in range(3):
        packed[channel::3] = bytes(frame[channel])
    with tempfile.TemporaryDirectory(prefix='vsr-ncnn-') as temp:
        source, output = os.path.join(temp, 'input.rgb'), os.path.join(temp, 'output.rgb')
        with open(source, 'wb') as f:
            f.write(packed)
        subprocess.run([os.path.join(runtime, 'python', 'python.exe'), os.path.join(runtime, 'ncnn-worker.py'),
                        '--model', entry['param'], '--scale', str(entry['scale']), '--width', str(c.width),
                        '--height', str(c.height), '--gpu', str(gpu), '--input', source, '--output', output],
                       check=True, timeout=600, creationflags=0x08000000, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        with open(output, 'rb') as f:
            pixels = f.read()
    width, height = c.width * entry['scale'], c.height * entry['scale']
    if len(pixels) != width * height * 3:
        raise ValueError('Invalid NCNN output size')
    blank = vs.core.std.BlankClip(rgb, width=width, height=height)
    def fill(n, f):
        out = f.copy()
        for channel in range(3):
            plane = pixels[channel::3]
            address = ctypes.addressof(ctypes.c_char.from_buffer(out[channel]))
            for y in range(height):
                ctypes.memmove(address + y * out.get_stride(channel), plane[y * width:(y + 1) * width], width)
        return out
    return vs.core.std.ModifyFrame(blank, blank, fill)
