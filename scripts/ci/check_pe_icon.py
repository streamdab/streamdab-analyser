#!/usr/bin/env python3
"""Check that Windows executables carry an application icon.

Usage: check_pe_icon.py <exe> [<exe> ...]

Each file must have an RT_GROUP_ICON resource that points at several RT_ICON
images (the .ico has 16..256 px frames). Requires `pip install pefile`.
"""
import sys

import pefile

RT_ICON = 3
RT_GROUP_ICON = 14


def count(pe, kind):
    entries = getattr(pe, "DIRECTORY_ENTRY_RESOURCE", None)
    if entries is None:
        return 0
    for top in entries.entries:
        if top.id == kind:
            return sum(len(sub.directory.entries) for sub in top.directory.entries)
    return 0


def main(paths):
    failed = False
    for path in paths:
        pe = pefile.PE(path, fast_load=False)
        groups, images = count(pe, RT_GROUP_ICON), count(pe, RT_ICON)
        ok = groups >= 1 and images >= 4
        print(f"{'ok  ' if ok else 'FAIL'} {path}: {groups} icon group(s), {images} image(s)")
        failed |= not ok
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    sys.exit(main(sys.argv[1:]))
