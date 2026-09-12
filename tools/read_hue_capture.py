import argparse
import base64
import collections
import datetime
import json
import pathlib
import random
import time


def analyze(raw):
    bits = [(value >> 2) & 1 for value in raw]
    changes = [i for i in range(1, len(bits)) if bits[i] != bits[i - 1]]
    rises = [i for i in changes if bits[i] == 1]
    runs = [(bits[a], b - a) for a, b in zip(changes, changes[1:])]
    return {
        'samples': len(bits), 'ones': sum(bits), 'rising_edges': len(rises),
        'high_samples': dict(collections.Counter(n for bit, n in runs if bit)),
        'low_samples': dict(collections.Counter(n for bit, n in runs if not bit)),
        'rise_intervals_samples': dict(collections.Counter(b-a for a, b in zip(rises, rises[1:]))),
    }


def read_exact(port, count):
    out = bytearray()
    deadline = time.monotonic() + 4
    while len(out) < count and time.monotonic() < deadline:
        out.extend(port.read(count-len(out)))
    if len(out) != count:
        raise RuntimeError(f'Incomplete frame: {len(out)}/{count}')
    return bytes(out)


def main():
    import serial
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', required=True)
    parser.add_argument('--count', type=int, default=40)
    parser.add_argument('--output', required=True)
    parser.add_argument('--trigger', action='store_true')
    parser.add_argument('--interval', type=float, default=0.0)
    parser.add_argument('--coarse', action='store_true')
    args = parser.parse_args()
    frames = []
    with serial.Serial(args.port, 115200, timeout=0.5, write_timeout=2, exclusive=True) as port:
        time.sleep(2)
        port.reset_input_buffer()
        port.write(b'?')
        version = port.readline().decode('ascii', errors='replace').strip()
        if not version.startswith('HUE_CAPTURE_V1'):
            raise RuntimeError(f'Wrong firmware reply: {version!r}')
        print(version, flush=True)
        for i in range(args.count):
            port.write(b'G' if args.coarse else b'T' if args.trigger else b'C')
            deadline = time.monotonic() + 3
            header = b''
            while not header and time.monotonic() < deadline:
                header = port.readline().strip()
            expected = b'CAPTURE_V1 1024 503 16000000 2' if args.coarse else b'CAPTURE_V1 1024 3 16000000 2'
            if header != expected:
                raise RuntimeError(f'Unexpected capture header {header!r}')
            raw = read_exact(port, 1024)
            if read_exact(port, 7) != b'\r\nEND\r\n':
                raise RuntimeError('Capture trailer mismatch')
            stats = analyze(raw)
            frames.append({'index': i, 'data_base64': base64.b64encode(raw).decode(), **stats})
            time.sleep(args.interval + random.uniform(0.005, 0.045))
    summary = {'frames': len(frames), 'active_frames': sum(f['rising_edges'] > 0 for f in frames),
               'observed_rising_edges': sum(f['rising_edges'] for f in frames)}
    for key in ('high_samples', 'low_samples', 'rise_intervals_samples'):
        counts = collections.Counter()
        for frame in frames:
            counts.update(frame[key])
        summary[key] = dict(sorted(counts.items()))
    result = {'timestamp_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'firmware': version, 'sample_period_ns': 31437.5 if args.coarse else 187.5,
              'acquisition_mode': 'coarse envelope; individual bits undersampled' if args.coarse else 'fast waveform',
              'window_us': 32192 if args.coarse else 192, 'scope': 'Discontinuous snapshots, not a continuous pulse count. Coarse mode cannot measure individual pulse widths or bit counts. Nominal16MHz. Logic levels only; no voltage measurement.',
              'user_reported_wiring': {'D2': 'verified strip DATA', 'GND': 'verified common ground', 'supply': 'strip supply must stay disconnected from Arduino IO'},
              'summary': summary, 'frames': frames}
    pathlib.Path(args.output).write_text(json.dumps(result, indent=2))
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
