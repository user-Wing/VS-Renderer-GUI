"""Summarize fixed local playback windows, excluding startup and seek."""
import csv
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
directory = root / 'build/performance'
fps = 7001 / 146
for kind, warmup, expected_runs in [('native', 5, 15), ('gui', 0, 9)]:
    results = []
    for path in sorted(directory.glob(f'final-{kind}-*-r*.csv')):
        if path.stem.endswith('-system'):
            continue
        with path.open(encoding='utf-8-sig') as file:
            rows = list(csv.DictReader(file))
        if not rows:
            continue
        window = [row for row in rows if float(row['wall_s']) >= warmup]
        if not window:
            continue
        first, last = window[0], window[-1]
        elapsed = float(last['wall_s']) - float(first['wall_s'])
        delta = {key: int(last[key]) - int(first[key]) for key in
                 ('decoded', 'accepted', 'dropped', 'coalesced', 'presents', 'seek_generation')}
        width, height = ('swap_w', 'swap_h') if kind == 'native' else ('width', 'height')
        expected_size = ('1920', '1080') if '1080-native' in path.stem else ('3840', '2160')
        source_s = float(last['position_s']) - float(first['position_s'])
        valid = (abs(elapsed - 60) < .2 and abs(source_s - elapsed) < .2 and
                 all((row[width], row[height]) == expected_size and
                     row['decode_mode'] == '2' and row['scaling_mode'] == '1' and
                     row['seek_generation'] == first['seek_generation'] for row in window))
        passed = (valid and delta['dropped'] == delta['coalesced'] == 0 and
                  abs(delta['accepted'] - elapsed * fps) < 3 and
                  delta['presents'] + 2 >= delta['accepted'])
        extra = {}
        if kind == 'gui':
            extra['audio_underruns'] = int(last['audio_underruns']) - int(first['audio_underruns'])
            extra['subtitle_visible'] = last['subtitle_visible'] == '1'
            extra['capture_exists'] = path.with_suffix('.csv.png').exists()
            test_log = path.with_name(path.stem + '-tests.txt').read_text(encoding='utf-8', errors='replace')
            extra['test_passed'] = '3 passed, 0 failed, 0 skipped' in test_log
            passed &= extra['audio_underruns'] == 0 and extra['capture_exists'] and extra['test_passed']
            if '4k10' in path.stem or '8k10' in path.stem:
                passed &= extra['subtitle_visible']
        else:
            extra['output_bits'] = int(last['output_bits'])
        results.append(dict(case=path.stem, elapsed_s=elapsed, valid_window=valid,
                            passed=bool(passed), source_s=source_s,
                            accepted_fps=delta['accepted']/elapsed if elapsed else 0,
                            present_fps=delta['presents']/elapsed if elapsed else 0,
                            extra_presents=delta['presents']-delta['accepted'],
                            output=f'{last[width]}x{last[height]}',
                            first_frame_s=float(last['first_frame_s']) if kind == 'native'
                            else float(last['first_frame_ms'])/1000, **delta, **extra))
    summary = dict(expected_runs=expected_runs, completed_runs=len(results),
                   passed=len(results) == expected_runs and all(row['passed'] for row in results),
                   results=results)
    (directory / f'final-{kind}-summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
    print(json.dumps(summary))

references = []
for path in sorted(directory.glob('final-mpv-*-r1.csv')):
    with path.open(encoding='utf-8-sig') as file:
        window = [row for row in csv.DictReader(file) if float(row['wall_s']) >= 5]
    if not window:
        continue
    first, last = window[0], window[-1]
    elapsed = float(last['wall_s']) - float(first['wall_s'])
    source_s = float(last['position_s']) - float(first['position_s'])
    valid = (abs(elapsed - 60) < .2 and abs(source_s - elapsed) < .2 and
             all(row['hwdec'] == 'd3d11va' and (row['width'], row['height']) == ('3840', '2160')
                 and 59 < float(row['display_fps']) < 61 for row in window))
    dropped = int(last['dropped']) - int(first['dropped'])
    decoder_dropped = int(last['decoder_dropped']) - int(first['decoder_dropped'])
    references.append(dict(case=path.stem, elapsed_s=elapsed, source_s=source_s,
                           valid_window=valid, dropped=dropped, decoder_dropped=decoder_dropped))
summary = dict(expected_runs=3, completed_runs=len(references), results=references)
(directory / 'final-mpv-summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
print(json.dumps(summary))
