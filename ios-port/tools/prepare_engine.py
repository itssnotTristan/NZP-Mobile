#!/usr/bin/env python3
"""Prepare an isolated, checksum-checked iOS engine tree; never edit Android inputs.

Usage: python3 tools/prepare_engine.py --android-root /path/to/android-port-v4-revised
The root must contain app/src/main/cpp/config_nzp_mobile.h and vendor/idtech4a.
The default SDL archive is checked against the supplied provenance manifest.
No network requests, executable downloads, signing, or publication occur here.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GROUPS = ('FTE_GL_FILES', 'FTE_QCVM_FILES', 'FTE_COMMON_FILES', 'FTE_SERVER_FILES', 'FTE_CLIENT_FILES')


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def copy_sources(source: Path, dest: Path) -> None:
    if not source.is_dir():
        raise ValueError(f'Missing source directory: {source}')
    # Never copy prior products or source control metadata into build inputs.
    shutil.copytree(source, dest, ignore=shutil.ignore_patterns(
        '.git', 'build', '.cxx', '__pycache__', '*.o', '*.a', '*.so', '*.dylib', '*.class'))


def apply_patches(engine: Path) -> dict:
    specification = json.loads((ROOT / 'patches/engine-ios.json').read_text())
    result = {}
    for entry in specification['files']:
        path = engine / entry['path']
        raw = path.read_bytes()
        if digest(raw) != entry['sha256']:
            raise ValueError(f"Canonical source mismatch before patch: {entry['path']}")
        text = raw.decode('utf-8')
        for change in entry['edits']:
            if text.count(change['old']) != 1:
                raise ValueError(f"Non-unique patch anchor: {entry['path']}")
            text = text.replace(change['old'], change['new'], 1)
        out = text.encode('utf-8')
        if digest(out) != entry['patched_sha256']:
            raise ValueError(f"Patched source hash mismatch: {entry['path']}")
        path.write_bytes(out)
        result[entry['path']] = {'source_sha256': entry['sha256'], 'patched_sha256': digest(out)}
    return result


def extract_source_list(engine: Path, out: Path) -> int:
    text = (engine / 'CMakeLists.txt').read_text()
    variables = {'FTE_VK_FILES': []}  # GLES-only. Never compile Vulkan/Android adapters.
    for name in GROUPS:
        matches = re.findall(r'^SET\(' + name + r'\s*\n(.*?)^\)', text, re.M | re.S)
        if len(matches) != 1:
            raise ValueError(f'Expected one source list for {name}, got {len(matches)}')
        sources = []
        for line in matches[0].splitlines():
            line = line.split('#', 1)[0].strip()
            if not line:
                continue
            for token in line.split():
                if token.startswith('${'):
                    variable = token[2:-1]
                    if variable not in variables:
                        raise ValueError(f'Unexpected nested source group: {variable}')
                    sources.extend(variables[variable])
                elif token.endswith(('.c', '.cpp', '.m')):
                    if not token.startswith('engine/') or not (engine / token).is_file():
                        raise ValueError(f'Unrecognized source: {token}')
                    sources.append(token)
        variables[name] = sources
    ordered = list(dict.fromkeys(variables['FTE_COMMON_FILES'] + variables['FTE_SERVER_FILES'] + variables['FTE_CLIENT_FILES']))
    forbidden = ('sys_android', 'vidandroid', 'q3e/', 'pr_x86', '/vk/')
    if any(any(marker in item for marker in forbidden) for item in ordered):
        raise ValueError('Forbidden Android/JIT/Vulkan source in generated list')
    out.write_text('# Generated from canonical source SET lists; no upstream CMake execution.\n'
                   'set(NZP_FTE_SOURCES\n' + ''.join(f'  "${{NZP_FTE_ROOT}}/{name}"\n' for name in ordered) + ')\n')
    return len(ordered)


def stage_sdl(work: Path, supplied: Path | None) -> None:
    destination = work / 'SDL2-source'
    manifest = json.loads((ROOT / 'provenance/sdl2-source.json').read_text())
    if supplied:
        copy_sources(supplied, destination)
    else:
        archive = ROOT / 'vendor-sources' / manifest['archive']
        if digest(archive.read_bytes()) != manifest['sha256']:
            raise ValueError('SDL archive checksum does not match provenance')
        with tarfile.open(archive, 'r:gz') as handle:
            # Archives must consist only of regular files/directories in SDL2/.
            members = handle.getmembers()
            for member in members:
                parts = Path(member.name).parts
                if (not parts or parts[0] != 'SDL2' or '..' in parts or
                        member.name.startswith('/') or not (member.isfile() or member.isdir())):
                    raise ValueError(f'Unsafe SDL archive member: {member.name}')
            handle.extractall(work, members=members, filter='data')
        (work / 'SDL2').rename(destination)
    for file in manifest['files']:
        if digest((destination / file['path']).read_bytes()) != file['sha256']:
            raise ValueError(f"SDL member checksum mismatch: {file['path']}")
    cmake = destination / 'CMakeLists.txt'
    text = cmake.read_text()
    marker = 'if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})'
    if not text.startswith('set(Q3E TRUE)\n') or text.count(marker) != 1:
        raise ValueError('Unexpected SDL vendor prologue; refusing unbounded alteration')
    # Remove only the Q3E-specific build prologue, retaining upstream detection.
    text = '# iOS staging: removed Android Q3E-only build prologue.\n' + text[text.index(marker):]
    cmake.write_text(text)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--android-root', type=Path, default=os.environ.get('NZP_ANDROID_SOURCE'))
    parser.add_argument('--sdl-source', type=Path, help='Optional local idTech4A SDL2 2.32.5 source tree')
    parser.add_argument('--output', type=Path, default=ROOT / 'build')
    args = parser.parse_args()
    if not args.android_root:
        parser.error('--android-root or NZP_ANDROID_SOURCE is required')
    android = Path(args.android_root).resolve()
    output = args.output.resolve()
    # Refuse to replace any source input, or write staging into the Android app.
    if output == android or (android in output.parents and output != ROOT / 'build'):
        raise ValueError('Inside the Android repository, only this ios-port/build staging path is allowed')
    native = android / 'vendor/idtech4a/Q3E/src/main/jni'
    config = android / 'app/src/main/cpp/config_nzp_mobile.h'
    if not config.is_file() or not (native / 'fteqw/CMakeLists.txt').is_file():
        raise ValueError('Android root is not the expected NZP source layout')
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.ios-engine-prepare-', dir=output) as temp:
        work = Path(temp)
        engine = work / 'engine-source'
        copy_sources(native / 'fteqw', engine)
        for dependency in ('libogg', 'libvorbis', 'libjpeg', 'libpng'):
            copy_sources(native / 'deplibs' / dependency, work / 'dependencies' / dependency)
        stage_sdl(work, args.sdl_source.resolve() if args.sdl_source else None)
        changed = apply_patches(engine)
        source_count = extract_source_list(engine, work / 'fte_sources.cmake')
        # Preserve canonical game/format settings, explicitly constrain runtime.
        disabled = ('VKQUAKE', 'AVAIL_OPENAL', 'AVAIL_DINPUT', 'AVAIL_DSOUND',
            'AVAIL_WASAPI', 'PLUGINS', 'USE_SQLITE', 'SUBSERVERS', 'PACKAGEMANAGER',
            'SUPPORT_ICE', 'WEBCLIENT', 'HAVE_GNUTLS', 'HAVE_WINSSPI', 'HAVE_HTTPSV',
            'FTPSERVER', 'VOICECHAT', 'HAVE_SPEEX', 'HAVE_OPUS', 'HAVE_CDPLAYER',
            'HAVE_MEDIA_DECODER', 'HAVE_MEDIA_ENCODER', 'TEXTEDITOR', 'CL_MASTER',
            'RAGDOLL', 'AVAIL_BOTLIB', 'VM_Q1', 'SVRANKING', 'NZP_MOBILE_TELEMETRY')
        (work / 'config_nzp_ios.h').write_text(config.read_text() +
            '\n/* Isolated iOS build profile: native SDL2/GLES, offline/local game. */\n' +
            ''.join('#undef ' + item + '\n' for item in disabled) + '\n#define GLESONLY\n#define GLSLONLY\n')
        java_config = android / 'app/src/main/java/org/nzp/mobile/preview/MobileConfig.java'
        java_text = java_config.read_text()
        config_match = re.search(r'static final String CONFIG\s*=\s*(.*?);', java_text, re.S)
        if not config_match:
            raise ValueError('Missing canonical MobileConfig.CONFIG')
        literals = re.findall(r'"(?:[^"\\]|\\.)*"', config_match[1])
        mobile_config = ''.join(json.loads(item) for item in literals)
        default_match = re.search(r'out\.write\(("(?:[^"\\]|\\.)*")\.getBytes', java_text)
        if not default_match or not mobile_config.startswith('// Generated NZP Mobile bindings.'):
            raise ValueError('Unexpected canonical mobile config/defaults')
        initial_config = json.loads(default_match[1])
        (work / 'nzp_ios_mobile_config.h').write_text(
            '/* Generated only from trusted canonical MobileConfig.java literals. */\n'
            '#define NZP_IOS_MOBILE_CONFIG ' + json.dumps(mobile_config) + '\n'
            '#define NZP_IOS_FIRST_RUN_CONFIG ' + json.dumps(initial_config) + '\n')
        manifest = {'schema': 1, 'android_config_sha256': digest(config.read_bytes()),
                    'source_count': source_count, 'patches': changed,
                    'mobile_config_source_sha256': digest(java_config.read_bytes()),
                    'scope': 'source preparation only; no Apple SDK build or runtime validation'}
        (work / 'engine-preparation.json').write_text(json.dumps(manifest, indent=2) + '\n')
        for name in ('engine-source', 'SDL2-source', 'dependencies', 'fte_sources.cmake',
                     'config_nzp_ios.h', 'nzp_ios_mobile_config.h', 'engine-preparation.json'):
            target = output / name
            if target.is_symlink():
                raise ValueError(f'Refusing to replace staging symlink: {target}')
            if target.is_dir():
                shutil.rmtree(target)
            elif target.exists():
                target.unlink()
            (work / name).rename(target)
    print(f'Prepared {source_count} FTE core sources, SDL2 and four static codec dependencies in {output}')
    print('Android inputs unchanged. Apple SDK compilation and device checks remain required.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
