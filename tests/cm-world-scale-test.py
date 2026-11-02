"""Run CM's production world Option/INI/load/save/reset migration behavior."""
from pathlib import Path
import os,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
subprocess.run([sys.executable,'-B',str(r/'tests/captured-defaults-test.py'),str(r),str(o)],env=env,check=True)
exe=o/'cm-world-scale-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(r/'core'),'-I'+str(r/'core/rend/vr'),str(Path(__file__).with_suffix('.cpp')),'-o',str(exe)],env=env,check=True)
sys.exit(subprocess.run([str(exe)],env=env).returncode)
