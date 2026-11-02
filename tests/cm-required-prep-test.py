"""Execute CM's real preparation state machine with controlled engine/cache boundaries."""
from pathlib import Path
import os,re,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
s=(r/'core/rend/vr/hands_rip.cpp').read_text(encoding='utf-8-sig')
def block(sig):
 a=s.index(sig);b=s.index('{',a);e=b+1;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
g=s[s.index('constexpr u32 MinGunPolys'):s.index('double gameSeconds()')]
functions='\n'.join(block(x) for x in ['std::string gunPath(', 'bool haveModel()', 'bool attractGunsLeft(', 'void logWatched(', 'void stopPrep(', 'void onStart()', 'void onVBlank()', 'bool preparing()', 'float prepProgress()', 'void skipPrep()'])
for sig,fallback in [('bool prepFailed()','bool prepFailed(){return false;}'),('std::string prepFailure()','std::string prepFailure(){return {};}')]:functions+='\n'+(block(sig) if sig in s else fallback)
# saveGun's first occurrence is a declaration; select its actual body.
save=s.index('bool saveGun(',s.index('// One of Confidential Mission'))
tail=s[save:];b=tail.index('{');e=b+1;d=1
while d:d+=(tail[e]=='{')-(tail[e]=='}');e+=1
functions+='\n'+tail[:e]
(o/'prep-paths.inc').write_text(block('std::string modelPath()')+'\n'+block('std::string legacyModelPath()'),encoding='utf8')
(o/'prep-production.inc').write_text(g+'\n'+functions,encoding='utf8')
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
exe=o/'cm-required-prep-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(r/'core'),'-I'+str(r/'core/rend/vr'),'-I'+str(r/'core/deps/glm'),str(Path(__file__).with_suffix('.cpp')),'-o',str(exe)],env=env,check=True)
sys.exit(subprocess.run([str(exe)],env=env).returncode)
