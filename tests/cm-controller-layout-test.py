from pathlib import Path
import os,subprocess,zipfile,json,sys
r=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1];o=Path(sys.argv[2]) if len(sys.argv)>2 else r/'build-win/vrtest/controller-layout';o.mkdir(parents=True,exist_ok=True)
s=(r/'core/rend/vr/xr_labels.cpp').read_text()
def block(s,signature):
 a=s.index(signature);b=s.index('{',a);e=b+1;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
part=s[s.index('using controllers::Button;'):s.index('// Where each hand\'s label lines end')]
part+='\n'+block(s,'void facing(')+'\n'+block(s,'void layOut(')
names=['a','b','x','y','menu','stick','trigger','grip']
anchors=[]
for hand in ['left','right']:
 d=json.loads((r/f'cm-vr/models/controllers/{hand}.json').read_text())
 assert d['aim']['rotation']==[0,1,0,0] and d['scale']==1
 # labelPoint uses inverse(aim node): the bundled GLB sidecars rotate180Y.
 anchors.append('{'+','.join('glm::vec3('+','.join(str(float(v)*(-1 if k in [0,2] else 1))+'f' for k,v in enumerate(d['buttons'].get(n,[0,0,0])))+')' for n in names)+'}')
(o/'layout.inc').write_text(part+'\nconst glm::vec3 Anchors[2][8]={'+','.join(anchors)+'};')
with zipfile.ZipFile(r/'fonts/Roboto-Bold.ttf.zip') as z:(o/'Roboto-Bold.ttf').write_bytes(z.read(z.namelist()[0]))
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
cmd=['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-static','-I'+str(o),'-I'+str(r/'core/deps/glm'),'-I'+str(r/'core/deps'),'-I'+str(r/'build-win/_deps/openxr_sdk-src/include'),'-I'+str(r/'core/rend/vr'),str(Path(__file__).with_suffix('.cpp')),'-o',str(o/'labels-test.exe')]
subprocess.run(cmd,check=True,env=env);sys.exit(subprocess.run([str(o/'labels-test.exe'),str(o/'Roboto-Bold.ttf')],env=env).returncode)
