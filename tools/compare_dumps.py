"""Compare two record dumps: missing names, count mismatches, word mismatches, max numeric deviation.

A record is one line: a name, then numbers (coordinates, volumes, areas) and words (member names,
field labels), as the compas_tf references in data/reference/floor/ and the floor example dumps in
data/output/pb/ write them. The name is the first token, or the first two when the second is an
index followed by a word (`wedge 0 inner_beams_0_0 ...`). Words must match exactly, numbers within
the tolerance.

    python3 tools/compare_dumps.py <reference> <candidate> [tolerance] [--relative <ratio>]

The tolerance is absolute, 1e-6 by default; --relative widens it by <ratio> times the magnitude of
the reference value, for the volume records (1e-9 of 1e8 mm3 is 0.1 mm3). Prints one line per
failing record, then `<N> records, <F> failing, worst deviation <D>`, and exits 1 when any fails.
"""

import re
import sys

INDEX = re.compile(r"^-?\d+$")


def number(token):
    """The token as a float, or None when it is a word."""
    try:
        return float(token)
    except ValueError:
        return None


def record_name(parts):
    """The first token, or the first two when the second is an index and a word follows."""
    if len(parts) > 2 and INDEX.match(parts[1]) and number(parts[2]) is None:
        return parts[0] + " " + parts[1]
    return parts[0]


def read(path):
    """Every record of the file by name: its words and its numbers in file order."""
    records = {}
    for line in open(path):
        parts = line.split()
        if not parts:
            continue
        name = record_name(parts)
        rest = parts[len(name.split()):]
        words = [token for token in rest if number(token) is None]
        numbers = [number(token) for token in rest if number(token) is not None]
        records[name] = (words, numbers)
    return records


def compare(reference, candidate, tolerance, ratio):
    """Prints every failing record; returns the failure count and the worst absolute deviation."""
    worst = 0.0
    failures = 0
    for name, (words, values) in reference.items():
        if name not in candidate:
            print("missing   ", name)
            failures += 1
            continue
        other_words, other = candidate[name]
        if len(other) != len(values) or len(other_words) != len(words):
            print("count     ", name, len(values), "vs", len(other), "values")
            failures += 1
            continue
        if other_words != words:
            print("words     ", name, " ".join(words), "vs", " ".join(other_words))
            failures += 1
            continue
        deviation = max((abs(a - b) for a, b in zip(values, other)), default=0.0)
        worst = max(worst, deviation)
        if any(abs(a - b) > tolerance + ratio * abs(a) for a, b in zip(values, other)):
            print("deviation ", name, "%.3e" % deviation)
            failures += 1
    for name in candidate:
        if name not in reference:
            print("extra     ", name)
    return failures, worst


def arguments(argv):
    """The reference and candidate paths, the absolute tolerance and the relative ratio."""
    paths = []
    ratio = 0.0
    i = 0
    while i < len(argv):
        if argv[i] == "--relative":
            ratio = float(argv[i + 1])
            i += 2
        else:
            paths.append(argv[i])
            i += 1
    if len(paths) < 2:
        sys.exit(__doc__)
    tolerance = float(paths[2]) if len(paths) > 2 else 1e-6
    return paths[0], paths[1], tolerance, ratio


def main(argv):
    """Compares the two dumps named on the command line and prints the summary line."""
    reference_path, candidate_path, tolerance, ratio = arguments(argv)
    reference = read(reference_path)
    candidate = read(candidate_path)
    failures, worst = compare(reference, candidate, tolerance, ratio)
    relative = f", relative {ratio:g}" if ratio > 0 else ""
    print(f"{len(reference)} records, {failures} failing, worst deviation {worst:.3e} (tolerance {tolerance:g}{relative})")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
