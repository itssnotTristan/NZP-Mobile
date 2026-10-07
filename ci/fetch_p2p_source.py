#!/usr/bin/env python3
"""Fetch and safely stage the frozen matching P2P Android and iOS sources.

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

EXPECTED_SHA256 = 'e53a12207f9254cd15110ead50efd6d33acee1e6afcb71eec5fe78b8744f953f'
MAX_DOWNLOAD = 224 * 1024 * 1024
MAX_EXTRACTED = 400 * 1024 * 1024
ROOT_NAME = 'NZP-Mobilized-P2P-paired-source-final'
EXPECTED_BYTES = 198422204
ARCHIVE_URL = 'https://github.com/itssnotTristan/NZP-Mobile/releases/download/p2p-build-inputs/NZP-Mobilized-P2P-paired-source-final.zip'

def sha256(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    src=parser.add_mutually_exclusive_group()
    src.add_argument('--archive-url',default=ARCHIVE_URL)
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
            if args.archive_url != ARCHIVE_URL:
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
        if archive.stat().st_size != EXPECTED_BYTES: raise ValueError('Source ZIP byte count does not match frozen P2P input')
        if sha256(archive)!=EXPECTED_SHA256: raise ValueError('Source ZIP SHA-256 does not match frozen P2P input')
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
                    not p.parts or p.parts[0]!=ROOT_NAME or str(p) in seen or
                    (stat.S_IFMT(mode) not in (0,stat.S_IFREG,stat.S_IFDIR))):
                    raise ValueError('Unsafe or duplicate ZIP member: '+entry.filename)
                seen.add(str(p))
                target=stage.joinpath(*p.parts)
                if entry.is_dir(): target.mkdir(parents=True,exist_ok=True);continue
                target.parent.mkdir(parents=True,exist_ok=True)
                with z.open(entry) as source,target.open('xb') as output: shutil.copyfileobj(source,output)
        root=stage/ROOT_NAME
        for required in ('android-port-p2p/app/src/main/assets/content-manifest.sha256', 'android-port-p2p/app/src/main/assets/game/nzp/mobile-network.cfg', 'ios-port/platform/apple/NZPPlayerProfile.m', 'ios-port/tools/build_unsigned.sh'):
            if not (root/required).is_file(): raise ValueError('Paired P2P source archive has unexpected layout: '+required)
        stage.rename(dest)
    source_root=dest/ROOT_NAME
    if args.github_env:
        env=os.environ.get('GITHUB_ENV')
        if not env or any(c in str(source_root) for c in '\r\n'): raise ValueError('Missing or unsafe GitHub env destination')
        with open(env,'a') as f:
            f.write('NZP_ANDROID_SOURCE='+str(source_root/'android-port-p2p')+'\n')
            f.write('NZP_IOS_SOURCE='+str(source_root/'ios-port')+'\n')
    print('Verified source ZIP SHA-256: '+EXPECTED_SHA256)
    print('NZP_ANDROID_SOURCE='+str(source_root/'android-port-p2p'))
    print('NZP_IOS_SOURCE='+str(source_root/'ios-port'))

if __name__=='__main__': main()
