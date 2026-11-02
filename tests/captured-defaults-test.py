from pathlib import Path
import subprocess,os,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
# Generate the existing actual Option/INI/menu fixture. Its separate baseline
# assertions are run by the full suite; generation completes before it executes.
p=subprocess.run([sys.executable,'-B',str(r/'tests/reticle-settings-test.py'),str(r),str(o)],capture_output=True,env=env)
if p.returncode:print(p.stdout.decode(),p.stderr.decode());sys.exit(p.returncode)
if not (o/'under_test.inc').exists():print(p.stdout.decode(),p.stderr.decode());sys.exit(2)
t=(r/'core/rend/vr/xr_menu.cpp').read_text(encoding='utf-8-sig');a=t.index('config::VrReticleMinSize.reset();',t.index('void resetMenuSettings()'));b=t.index('config::VrHaptics.reset();',a)
world=t[t.index('config::VrWorldScale.reset();',t.index('void resetMenuSettings()')):t.index('config::VrHudDepth.reset();',t.index('void resetMenuSettings()'))]
(o/'reset-captured.inc').write_text('void resetCaptured(){config::setAutoSave(false);\n'+t[a:b]+world+'\nconfig::setAutoSave(true);}',encoding='utf8')
binary=o/'captured-defaults.exe';cpp=Path(__file__).with_suffix('.cpp')
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(r/'core'),'-I'+str(r/'core/rend/vr'),str(cpp),'-o',str(binary)],check=True,env=env)
sys.exit(subprocess.run([str(binary)],env=env).returncode)
