#!/usr/bin/env python3
"""Read-only SHA256 verifier for locally obtained nabu/Mi9 driver sources."""

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "evidence" / "nabu-mi9-driver-sources-20261028.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--set", choices=("original", "mi9", "local-pep", "archive"), required=True)
    parser.add_argument("path", type=Path, help="driver directory, or 7z file with --set archive")
    args = parser.parse_args()
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))

    if args.set == "archive":
        if not args.path.is_file():
            parser.error("archive path must be a file")
        actual = sha256(args.path)
        expected = manifest["originalBsp"]["sha256"]
        print(f"{'OK' if actual == expected else 'MISMATCH'} {args.path.name} {actual}")
        return 0 if actual == expected else 1

    if not args.path.is_dir():
        parser.error("driver path must be a directory")
    field = {"original": "originalBspSha256", "mi9": "mi9UpstreamSha256", "local-pep": "localPatchedSha256"}[args.set]
    components = manifest["components"]
    if args.set == "local-pep":
        components = [item for item in components if item["name"] == "PEP-CPU7-Guard"]
    failed = False
    for item in components:
        found = list(args.path.rglob(item["file"]))
        if len(found) != 1:
            print(f"MISSING_OR_AMBIGUOUS {item['file']} matches={len(found)}")
            failed = True
            continue
        actual = sha256(found[0])
        expected = item[field]
        good = actual == expected
        print(f"{'OK' if good else 'MISMATCH'} {item['file']} {actual}")
        failed |= not good
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
