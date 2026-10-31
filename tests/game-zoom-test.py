from pathlib import Path
import os,subprocess,sys
root=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
binary=out/'game-zoom-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(root/'core/rend/vr'),str(Path(__file__).with_suffix('.cpp')),'-o',str(binary)],env=env,check=True)
sys.exit(subprocess.run([str(binary)],env=env).returncode)
