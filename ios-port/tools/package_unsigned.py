#!/usr/bin/env python3
"""Package only an actual Mach-O iOS app. This does not sign or install it."""
from pathlib import Path
import plistlib,sys,zipfile

app=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve()
if not app.is_dir() or app.suffix!='.app': raise SystemExit('Expected a built .app directory')
info=plistlib.loads((app/'Info.plist').read_bytes())
exe=app/info['CFBundleExecutable']
if exe.read_bytes()[:4] not in (b'\xcf\xfa\xed\xfe',b'\xfe\xed\xfa\xcf',b'\xca\xfe\xba\xbe',b'\xbe\xba\xfe\xca'):
    raise SystemExit('App executable is not a 64-bit Mach-O or universal binary')
if (app/'embedded.mobileprovision').exists(): raise SystemExit('This packaging path is for unsigned builds only')
out.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in sorted(app.rglob('*')):
        if p.is_symlink(): raise SystemExit('Unexpected app bundle symlink: '+str(p))
        if p.is_file(): z.write(p,'Payload/'+app.name+'/'+str(p.relative_to(app)))
print(out)
