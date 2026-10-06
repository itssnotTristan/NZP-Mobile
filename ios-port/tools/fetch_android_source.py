#!/usr/bin/env python3
"""Fetch only the reviewed corresponding v4 source ZIP and safely stage it.

No binary from the archive is executed. --archive supports offline verification.
The archive checksum pins the source bytes independently of GitHub redirects.
"""
import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import tempfile
import urllib.parse
import urllib.request
import zipfile

EXPECTED_SHA256 = '9b26376dc82d7ff796826b288d13902e9b1b67738ec15ba8274129b7d7992a4f'
MAX_DOWNLOAD = 160 * 1024 * 1024
MAX_EXTRACTED = 400 * 1024 * 1024
ROOT_NAME = 'NZP-Mobile-0.4.0-preview-source'

def sha256(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    src=parser.add_mutually_exclusive_group(required=True)
    src.add_argument('--archive-url')
    src.add_argument('--archive',type=Path)
    parser.add_argument('--destination',type=Path,required=True)
    parser.add_argument('--github-env',action='store_true')
    args=parser.parse_args()
    dest=args.destination.resolve()
    if dest.exists(): raise ValueError('Source destination already exists; choose a fresh directory')
    dest.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.nzp-source-',dir=dest.parent) as temp:
        temp=Path(temp)
        if args.archive:
            archive=args.archive.resolve()
            if archive.stat().st_size>MAX_DOWNLOAD: raise ValueError('Archive too large')
        else:
            url=urllib.parse.urlsplit(args.archive_url)
            if url.scheme!='https' or url.hostname!='github.com' or '/releases/download/' not in url.path:
                raise ValueError('Expected a verified HTTPS GitHub release asset URL')
            archive=temp/'source.zip'
            with urllib.request.urlopen(args.archive_url,timeout=90) as response,archive.open('wb') as out:
                count=0
                while True:
                    chunk=response.read(1024*1024)
                    if not chunk: break
                    count+=len(chunk)
                    if count>MAX_DOWNLOAD: raise ValueError('Download size limit exceeded')
                    out.write(chunk)
        if sha256(archive)!=EXPECTED_SHA256: raise ValueError('Source ZIP SHA-256 does not match reviewed v4 bytes')
        stage=temp/'extracted';stage.mkdir()
        with zipfile.ZipFile(archive) as z:
            entries=z.infolist()
            if len(entries)>8000 or sum(x.file_size for x in entries)>MAX_EXTRACTED:
                raise ValueError('Expanded source exceeds safety limits')
            seen=set()
            for entry in entries:
                p=PurePosixPath(entry.filename)
                mode=(entry.external_attr>>16)&0xffff
                if (p.is_absolute() or '..' in p.parts or '\\' in entry.filename or
                    not p.parts or p.parts[0]!=ROOT_NAME or entry.filename in seen or
                    (stat.S_IFMT(mode) not in (0,stat.S_IFREG,stat.S_IFDIR))):
                    raise ValueError('Unsafe or duplicate ZIP member: '+entry.filename)
                seen.add(entry.filename)
                target=stage.joinpath(*p.parts)
                if entry.is_dir(): target.mkdir(parents=True,exist_ok=True);continue
                target.parent.mkdir(parents=True,exist_ok=True)
                with z.open(entry) as source,target.open('xb') as output: shutil.copyfileobj(source,output)
        root=stage/ROOT_NAME
        if not (root/'app/src/main/assets/content-manifest.sha256').is_file():
            raise ValueError('Source archive has unexpected root layout')
        stage.rename(dest)
    source_root=dest/ROOT_NAME
    if args.github_env:
        env=os.environ.get('GITHUB_ENV')
        if not env or any(c in str(source_root) for c in '\r\n'): raise ValueError('Missing or unsafe GitHub env destination')
        with open(env,'a') as f:f.write('NZP_ANDROID_SOURCE='+str(source_root)+'\n')
    print('Verified source ZIP SHA-256: '+EXPECTED_SHA256)
    print('NZP_ANDROID_SOURCE='+str(source_root))

if __name__=='__main__': main()
