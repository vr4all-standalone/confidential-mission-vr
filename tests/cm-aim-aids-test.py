"""Exercise the actual CM stick-click block with production Option/INI settings."""
from pathlib import Path
import os, subprocess, sys

r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
# Existing runner compiles the real INI/Option/menu bodies and checks50 cases.
subprocess.run([sys.executable,'-B',str(r/'tests/reticle-settings-test.py'),str(r),str(o/'base')],env=env,check=True)
host=(r/'core/rend/vr/xr_host.cpp').read_text(encoding='utf-8-sig')
start=host.index('const bool laser = boolAction(laserAction);')
end=host.index('laserWas = laser;',start)+len('laserWas = laser;')
option=(r/'core/cfg/option.cpp').read_text(encoding='utf-8-sig')
laser=next(x for x in option.splitlines() if x.startswith('Option<bool> VrLaser('))
code='namespace config { '+laser+' }\nvoid sampleShortcut(){\n'+host[start:end]+'\n}\n'
(o/'shortcut.inc').write_text(code,encoding='utf8')
exe=o/'cm-aim-aids-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(r/'core'),'-I'+str(r/'core/rend/vr'),str(Path(__file__).with_suffix('.cpp')),'-o',str(exe)],env=env,check=True)
sys.exit(subprocess.run([str(exe)],env=env).returncode)
