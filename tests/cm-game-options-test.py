"""Compile and exercise the production CM native protocol model; no ROM or headset needed."""
from pathlib import Path
import os, shutil, subprocess, sys
root=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
compiler=os.environ.get("CXX") or shutil.which("g++") or "D:/msys64/mingw64/bin/g++.exe"
env=dict(os.environ);env["PATH"]=str(Path(compiler).parent)+os.pathsep+env.get("PATH","")
binary=out/"cm-game-options-test.exe"
subprocess.run([compiler,"-std=c++17","-O2","-I"+str(root/"core"),str(root/"tests/cm-game-options-test.cpp"),"-o",str(binary)],env=env,check=True)
sys.exit(subprocess.run([str(binary)],env=env).returncode)
