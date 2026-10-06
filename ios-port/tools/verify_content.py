#!/usr/bin/env python3
"""Verify bundled assets before an Apple build; never alter the Android tree."""
import hashlib
from pathlib import Path, PurePosixPath
import sys

def main():
    source=Path(sys.argv[1]).resolve()
    base=source/'app/src/main/assets'
    game=base/'game'
    count=0
    seen=set()
    for line in (base/'content-manifest.sha256').read_text().splitlines():
        digest,relative=line.split('  ',1)
        p=PurePosixPath(relative)
        if len(digest)!=64 or any(c not in '0123456789abcdef' for c in digest):
            raise ValueError('Invalid content digest')
        if p.is_absolute() or '..' in p.parts or relative in seen:
            raise ValueError('Unsafe or duplicate content path: '+relative)
        file=game.joinpath(*p.parts)
        if file.is_symlink() or not file.is_file() or not file.resolve().is_relative_to(game.resolve()):
            raise ValueError('Missing or unsafe content: '+relative)
        if hashlib.sha256(file.read_bytes()).hexdigest()!=digest:
            raise ValueError('Content checksum mismatch: '+relative)
        count+=1;seen.add(relative)
    actual={str(p.relative_to(game)) for p in game.rglob('*') if p.is_file()}
    if actual!=seen: raise ValueError('Content manifest does not cover exact game payload')
    print(f'PASS: {count} bundled game files verified')

if __name__=='__main__': main()
